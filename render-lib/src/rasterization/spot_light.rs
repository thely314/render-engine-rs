use std::f32::consts::PI;
use std::f32::INFINITY;
use std::thread;

use crate::rasterization::light::*;
use crate::rasterization::model::*;
use crate::util::math::*;
use nalgebra::clamp;
use nalgebra::{max, min};

use super::unsafe_pack::*;
const SPOT_LIGHT_MAXIMUM_THREAD_NUM: i32 = 8;
const SPOT_LIGHT_BIAS_SCALE: f32 = 0.05;
const SPOT_LIGHT_PCF_RADIUS: i32 = 1;
const SPOT_LIGHT_FIBONACCI_CLUMP_EXPONENT: f32 = 1.0;
const SPOT_LIGHT_SAMPLE_NUM: i32 = 64;

pub struct SpotLight {
    light: Light,
    light_dir: Vector3f,
    fov: f32,
    aspect_ratio: f32,
    pub(in crate::rasterization) z_near: f32,
    pub(in crate::rasterization) z_far: f32,
    z_buffer_width: i32,
    z_buffer_height: i32,
    penumbra_mask_width: i32,
    penumbra_mask_height: i32,
    light_size: f32,
    fov_factor: f32,
    pixel_radius: f32,
    enable_shadow: bool,
    enable_pcf_sample_accelerate: bool,
    enable_pcss_sample_accelerate: bool,
    enable_penumbra_mask: bool,
    mvp: Matrix4f,
    mv: Matrix4f,
    z_buffer: Vec<f32>,
    penumbra_mask: Vec<f32>,
}
impl Default for SpotLight {
    fn default() -> Self {
        SpotLight {
            light: Light::default(),
            light_dir: Vector3f::new(0.0, 0.0, -1.0),
            fov: 90.0,
            aspect_ratio: 1.0,
            z_near: -0.1,
            z_far: -1000.0,
            light_size: 1.0,
            fov_factor: 0.0,
            pixel_radius: 0.0,
            z_buffer_width: 2048,
            z_buffer_height: 2048,
            penumbra_mask_width: 0,
            penumbra_mask_height: 0,
            enable_shadow: true,
            enable_pcf_sample_accelerate: false,
            enable_pcss_sample_accelerate: true,
            enable_penumbra_mask: true,
            mvp: Matrix4f::identity(),
            mv: Matrix4f::identity(),
            z_buffer: Vec::default(),
            penumbra_mask: Vec::default(),
        }
    }
}

impl LightTrait for SpotLight {
    fn get_pos(&self) -> Vector3f {
        self.light.get_pos()
    }
    fn set_pos(&mut self, pos: Vector3f) {
        self.light.set_pos(pos);
    }
    fn get_intensity(&self) -> Vector3f {
        self.light.get_intensity()
    }
    fn set_intensity(&mut self, intensity: Vector3f) {
        self.light.set_intensity(intensity);
    }
    fn compute_world_light_dir(&self, point: Vector3f) -> Vector3f {
        (point - self.get_pos()).normalize()
    }
    fn compute_world_light_intensity(&self, point: Vector3f) -> Vector3f {
        self.get_intensity() / ((point - self.get_pos()).dot(&(point - self.get_pos())))
    }
    fn look_at(&mut self, models: *const Vec<*mut Model>) {
        if !self.enable_shadow {
            return;
        }
        self.z_buffer.resize(
            self.z_buffer_width as usize * self.z_buffer_height as usize,
            -INFINITY,
        );
        self.z_buffer.fill(-INFINITY);
        let modeling = Matrix4f::identity();
        let view = get_view_matrix(self.get_pos(), self.light_dir);
        let projection =
            get_projection_matrix(self.fov, self.aspect_ratio, self.z_near, self.z_far);
        self.mv = view * modeling;
        self.mvp = projection * self.mv;
        self.fov_factor = f32::tan(self.fov / 360.0 * PI);
        self.pixel_radius = f32::max(
            1.0 / self.z_buffer_height as f32,
            self.aspect_ratio / self.z_buffer_width as f32,
        );
        for model in unsafe { &*models } {
            let model_ref = unsafe { model.as_mut() }.unwrap();
            model_ref.clip(&self.mvp, &self.mv);
            model_ref.to_NDC(self.z_buffer_width as u32, self.z_buffer_height as u32);
        }
        let thread_num = min(self.z_buffer_height, SPOT_LIGHT_MAXIMUM_THREAD_NUM);
        let rows_per_thread = (self.z_buffer_height as f32 / thread_num as f32).ceil() as i32;
        let z_buffer_width = self.z_buffer_width;
        let z_buffer_height = self.z_buffer_height;
        let get_index_closure = move |x: i32, y: i32| z_buffer_width * y + x;
        let depth_transformer = move |z: f32, w: f32| w;
        let z_buffer_ptr = ZBufferPtr(&self.z_buffer as *const Vec<f32> as *mut Vec<f32>);
        let vec_mut_model_ptr = ConstVecMutModelPtr(models);
        let mut handles = Vec::with_capacity(thread_num as usize);
        for tid in 0..thread_num - 1 {
            let handle = thread::spawn(move || unsafe {
                let models = vec_mut_model_ptr.get();
                let z_buffer_ptr_raw: *mut Vec<f32> = z_buffer_ptr.get_mut();
                for model in &*models {
                    let model_ref = model.as_ref().unwrap();
                    model_ref.rasterization_shadow_map::<true>(
                        z_buffer_ptr_raw,
                        tid * rows_per_thread,
                        0,
                        rows_per_thread,
                        z_buffer_width,
                        &depth_transformer,
                        &get_index_closure,
                    );
                }
            });
            handles.push(handle);
        }
        let handle = thread::spawn(move || unsafe {
            let models = vec_mut_model_ptr.get();
            let z_buffer_ptr_raw: *mut Vec<f32> = z_buffer_ptr.get_mut();
            for model in &*models {
                let model_ref = model.as_ref().unwrap();
                model_ref.rasterization_shadow_map::<true>(
                    z_buffer_ptr_raw,
                    (thread_num - 1) * rows_per_thread,
                    0,
                    z_buffer_height - (thread_num - 1) * rows_per_thread,
                    z_buffer_width,
                    &depth_transformer,
                    &get_index_closure,
                );
            }
        });
        handles.push(handle);
        for handle in handles {
            handle.join().unwrap();
        }
    }

    fn in_shadow(&self, point: Vector3f, normal: Vector3f, shadow_method: ShadowMethod) -> f32 {
        match shadow_method {
            ShadowMethod::DIRECT => self.in_shadow_direct(point, normal),
            ShadowMethod::PCF => self.in_shadow_pcf(point, normal),
            ShadowMethod::PCSS => self.in_shadow_pcss(point, normal),
        }
    }
    fn in_penumbra_mask(&self, x: i32, y: i32) -> bool {
        if self.enable_shadow && self.enable_penumbra_mask {
            return self.penumbra_mask[self.get_penumbra_mask_index(x / 4, y / 4) as usize]
                > EPSILON;
        }
        true
    }
    fn generate_penumbra_mask(&mut self, scene: *const crate::rasterization::scene::Scene) {
        if !self.enable_shadow || !self.enable_penumbra_mask {
            return;
        }
        let scene_ref = unsafe { scene.as_ref().unwrap() };
        let penumbra_mask_width = (0.25 * scene_ref.width as f32).ceil() as i32;
        let penumbra_mask_height = (0.25 * scene_ref.height as f32).ceil() as i32;
        self.penumbra_mask_width = penumbra_mask_width;
        self.penumbra_mask_height = penumbra_mask_height;
        self.penumbra_mask.resize(
            penumbra_mask_width as usize * penumbra_mask_height as usize,
            0.0,
        );
        self.penumbra_mask.fill(0.0);
        let penumbra_mask_thread_num = min(penumbra_mask_height, SPOT_LIGHT_MAXIMUM_THREAD_NUM);
        let rows_per_thread =
            (penumbra_mask_height as f32 / penumbra_mask_thread_num as f32).ceil() as i32;
        let scene_ptr = ConstScenePtr(scene);
        let mut handles = Vec::with_capacity(penumbra_mask_thread_num as usize);
        let light_ptr = MutSpotLightPtr(self as *mut SpotLight);
        for tid in 0..penumbra_mask_thread_num - 1 {
            let handle = thread::spawn(move || unsafe {
                let light_ptr = light_ptr.get_mut();
                let scene_ptr = scene_ptr.get();
                (*light_ptr).generate_penumbra_mask_block(
                    scene_ptr,
                    tid * rows_per_thread,
                    0,
                    rows_per_thread,
                    penumbra_mask_width,
                );
            });
            handles.push(handle);
        }
        let handle = thread::spawn(move || unsafe {
            let light_ptr = light_ptr.get_mut();
            let scene_ptr = scene_ptr.get();
            (*light_ptr).generate_penumbra_mask_block(
                scene_ptr,
                (penumbra_mask_thread_num - 1) * rows_per_thread,
                0,
                penumbra_mask_height - (penumbra_mask_thread_num - 1) * rows_per_thread,
                penumbra_mask_width,
            );
        });
        handles.push(handle);
        for handle in handles {
            handle.join().unwrap();
        }
    }
    fn box_blur_penumbra_mask(&mut self, radius: i32) {
        if !self.enable_shadow || !self.enable_penumbra_mask {
            return;
        }
        let penumbra_mask_width = self.penumbra_mask_width;
        let get_index_closure = move |x: i32, y: i32| penumbra_mask_width * y + x;
        self.penumbra_mask = blur_penumbra_mask_vertical(
            &blur_penumbra_mask_horizontal(
                &self.penumbra_mask,
                self.penumbra_mask_width,
                self.penumbra_mask_height,
                radius,
                &get_index_closure,
            ),
            self.penumbra_mask_width,
            self.penumbra_mask_height,
            radius,
            &get_index_closure,
        );
    }
}
impl SpotLight {
    pub fn set_light_dir(&mut self, light_dir: Vector3f) {
        self.light_dir = light_dir.normalize();
    }
    pub fn get_light_dir(&self) -> Vector3f {
        self.light_dir
    }
    pub fn set_fov(&mut self, fov: f32) {
        self.fov = fov;
    }
    pub fn get_fov(&self) -> f32 {
        self.fov
    }
    pub fn set_aspect_ratio(&mut self, aspect_ratio: f32) {
        self.aspect_ratio = aspect_ratio;
    }
    pub fn get_aspect_ratio(&self) -> f32 {
        self.aspect_ratio
    }
    pub fn set_z_near(&mut self, z_near: f32) {
        self.z_near = z_near;
    }
    pub fn get_z_near(&self) -> f32 {
        self.z_near
    }
    pub fn set_z_far(&mut self, z_far: f32) {
        self.z_far = z_far;
    }
    pub fn get_z_far(&self) -> f32 {
        self.z_far
    }
    pub fn set_z_buffer_width(&mut self, z_buffer_width: i32) {
        self.z_buffer_width = z_buffer_width;
    }
    pub fn get_z_buffer_width(&self) -> i32 {
        self.z_buffer_width
    }
    pub fn set_z_buffer_height(&mut self, z_buffer_height: i32) {
        self.z_buffer_height = z_buffer_height;
    }
    pub fn get_z_buffer_height(&self) -> i32 {
        self.z_buffer_height
    }
    pub fn get_shadow_status(&self) -> bool {
        self.enable_shadow
    }

    pub fn set_shadow_status(&mut self, status: bool) {
        self.enable_shadow = status;
    }

    pub fn get_pcf_sample_accelerate_status(&self) -> bool {
        self.enable_pcf_sample_accelerate
    }

    pub fn set_pcf_sample_accelerate_status(&mut self, status: bool) {
        self.enable_pcf_sample_accelerate = status;
    }

    pub fn get_pcss_sample_accelerate_status(&self) -> bool {
        self.enable_pcss_sample_accelerate
    }

    pub fn set_pcss_sample_accelerate_status(&mut self, status: bool) {
        self.enable_pcss_sample_accelerate = status;
    }

    pub fn get_penumbra_mask_status(&self) -> bool {
        self.enable_penumbra_mask
    }

    pub fn set_penumbra_mask_status(&mut self, status: bool) {
        self.enable_penumbra_mask = status;
    }
    fn get_index(&self, x: i32, y: i32) -> i32 {
        self.z_buffer_width * y + x
    }
    fn get_penumbra_mask_index(&self, x: i32, y: i32) -> i32 {
        self.penumbra_mask_width * y + x
    }
    fn in_shadow_direct(&self, point: Vector3f, normal: Vector3f) -> f32 {
        if !self.enable_shadow {
            return 1.0;
        }
        let transform_pos = self.mvp * homogeneous(point);
        if transform_pos.x < transform_pos.w
            || transform_pos.x > -transform_pos.w
            || transform_pos.y < transform_pos.w
            || transform_pos.y > -transform_pos.w
            || transform_pos.z < transform_pos.w
            || transform_pos.z > -transform_pos.w
        {
            return 0.0;
        }
        let x_to_int = clamp(
            ((transform_pos.x / transform_pos.w + 1.0) * 0.5 * self.z_buffer_width as f32) as i32,
            0,
            self.z_buffer_width - 1,
        );
        let y_to_int = clamp(
            ((transform_pos.y / transform_pos.w + 1.0) * 0.5 * self.z_buffer_height as f32) as i32,
            0,
            self.z_buffer_height - 1,
        );
        let transform_pos = self.mv * homogeneous(point);
        let bias = f32::max(
            0.2,
            1.0 * (1.0 - (self.get_pos() - point).normalize().dot(&normal)),
        ) * SPOT_LIGHT_BIAS_SCALE
            * self.fov_factor
            * -transform_pos.z
            * 512.0
            * self.pixel_radius;
        if transform_pos.z + bias > self.z_buffer[self.get_index(x_to_int, y_to_int) as usize] {
            return 1.0;
        }
        0.0
    }
    fn in_shadow_pcf(&self, point: Vector3f, normal: Vector3f) -> f32 {
        if !self.enable_shadow {
            return 1.0;
        }
        let transform_pos = self.mvp * homogeneous(point);
        if transform_pos.x < transform_pos.w
            || transform_pos.x > -transform_pos.w
            || transform_pos.y < transform_pos.w
            || transform_pos.y > -transform_pos.w
            || transform_pos.z < transform_pos.w
            || transform_pos.z > -transform_pos.w
        {
            return 0.0;
        }
        let center_x = clamp(
            ((transform_pos.x / transform_pos.w + 1.0) * 0.5 * self.z_buffer_width as f32) as i32,
            0,
            self.z_buffer_width - 1,
        );
        let center_y = clamp(
            ((transform_pos.y / transform_pos.w + 1.0) * 0.5 * self.z_buffer_height as f32) as i32,
            0,
            self.z_buffer_height - 1,
        );
        let transform_pos = self.mv * homogeneous(point);
        let bias = f32::max(
            0.2,
            1.0 * (1.0 - (self.get_pos() - point).normalize().dot(&normal)),
        ) * SPOT_LIGHT_BIAS_SCALE
            * self.fov_factor
            * -transform_pos.z
            * 512.0
            * self.pixel_radius;
        let mut unshadow_num = 0;
        let pcf_radius = SPOT_LIGHT_PCF_RADIUS;
        if self.enable_pcf_sample_accelerate {
            let sample_count_inverse = 1.0 / SPOT_LIGHT_SAMPLE_NUM as f32;
            for i in 0..SPOT_LIGHT_SAMPLE_NUM {
                let sample_dir = compute_fibonacci_spiral_disk_sample_uniform(
                    i,
                    sample_count_inverse,
                    SPOT_LIGHT_FIBONACCI_CLUMP_EXPONENT,
                    0.0,
                );
                let x = (pcf_radius as f32 * sample_dir.x).round() as i32;
                let y = (pcf_radius as f32 * sample_dir.y).round() as i32;
                let idx_x = clamp(center_x + x, 0, self.z_buffer_width - 1);
                let idx_y = clamp(center_y + y, 0, self.z_buffer_height - 1);
                if transform_pos.z + (max(x.abs(), y.abs()) + 1) as f32 * bias
                    > self.z_buffer[self.get_index(idx_x, idx_y) as usize]
                {
                    unshadow_num += 1;
                }
            }
            unshadow_num as f32 / SPOT_LIGHT_SAMPLE_NUM as f32
        } else {
            for y in -pcf_radius..=pcf_radius {
                for x in -pcf_radius..=pcf_radius {
                    let idx_x = clamp(center_x + x, 0, self.z_buffer_width - 1);
                    let idx_y = clamp(center_y + y, 0, self.z_buffer_height - 1);
                    if transform_pos.z + (max(x.abs(), y.abs()) + 1) as f32 * bias
                        > self.z_buffer[self.get_index(idx_x, idx_y) as usize]
                    {
                        unshadow_num += 1;
                    }
                }
            }
            unshadow_num as f32 / (2 * pcf_radius + 1).pow(2) as f32
        }
    }
    fn in_shadow_pcss(&self, point: Vector3f, normal: Vector3f) -> f32 {
        if !self.enable_shadow {
            return 1.0;
        }
        let transform_pos = self.mvp * homogeneous(point);
        if transform_pos.x < transform_pos.w
            || transform_pos.x > -transform_pos.w
            || transform_pos.y < transform_pos.w
            || transform_pos.y > -transform_pos.w
            || transform_pos.z < transform_pos.w
            || transform_pos.z > -transform_pos.w
        {
            return 0.0;
        }
        let center_x = clamp(
            ((transform_pos.x / transform_pos.w + 1.0) * 0.5 * self.z_buffer_width as f32) as i32,
            0,
            self.z_buffer_width - 1,
        );
        let center_y = clamp(
            ((transform_pos.y / transform_pos.w + 1.0) * 0.5 * self.z_buffer_height as f32) as i32,
            0,
            self.z_buffer_height - 1,
        );
        let transform_pos = self.mv * homogeneous(point);
        let bias = f32::max(
            0.2,
            1.0 * (1.0 - (self.get_pos() - point).normalize().dot(&normal)),
        ) * SPOT_LIGHT_BIAS_SCALE
            * self.fov_factor
            * -transform_pos.z
            * 512.0
            * self.pixel_radius;
        //这个是试出来的
        let pcss_radius = f32::max(
            1.0,
            ((transform_pos.z + 1.0) / transform_pos.z * 0.5 * self.light_size
                / self.fov_factor
                / self.pixel_radius
                / 32.0)
                .round(),
        ) as i32;
        //这个也是试出来的
        //It just works
        let mut block_num = 0;
        let mut block_depth: f32 = 0.0;
        if self.enable_pcss_sample_accelerate {
            let sample_count_inverse = 1.0 / SPOT_LIGHT_SAMPLE_NUM as f32;
            for i in 0..SPOT_LIGHT_SAMPLE_NUM {
                let sample_dir = compute_fibonacci_spiral_disk_sample_uniform(
                    i,
                    sample_count_inverse,
                    SPOT_LIGHT_FIBONACCI_CLUMP_EXPONENT,
                    0.0,
                );
                let x = (pcss_radius as f32 * sample_dir.x).round() as i32;
                let y = (pcss_radius as f32 * sample_dir.y).round() as i32;
                let idx_x = clamp(center_x + x, 0, self.z_buffer_width - 1);
                let idx_y = clamp(center_y + y, 0, self.z_buffer_height - 1);
                if transform_pos.z + (max(x.abs(), y.abs()) + 1) as f32 * bias
                    < self.z_buffer[self.get_index(idx_x, idx_y) as usize]
                {
                    block_num += 1;
                    block_depth += self.z_buffer[self.get_index(idx_x, idx_y) as usize];
                }
            }
        } else {
            for y in -pcss_radius..=pcss_radius {
                for x in -pcss_radius..=pcss_radius {
                    let idx_x = clamp(center_x + x, 0, self.z_buffer_width - 1);
                    let idx_y = clamp(center_y + y, 0, self.z_buffer_height - 1);
                    if transform_pos.z + (max(x.abs(), y.abs()) + 1) as f32 * bias
                        < self.z_buffer[self.get_index(idx_x, idx_y) as usize]
                    {
                        block_num += 1;
                        block_depth += self.z_buffer[self.get_index(idx_x, idx_y) as usize];
                    }
                }
            }
        }
        if block_num == 0 || block_depth > -EPSILON {
            return 1.0;
        }
        block_depth /= block_num as f32;
        let penumbra = (transform_pos.z - block_depth) / block_depth * self.light_size;
        let pcf_radius = f32::max(
            1.0,
            (0.5 * penumbra / -transform_pos.z / self.fov_factor / self.pixel_radius).round(),
        ) as i32;
        //这个是算出来的(除去为了适应不同的宽高比而取的self.pixel_radius)
        let mut unshadow_num = 0;
        if self.enable_pcf_sample_accelerate {
            let sample_count_inverse = 1.0 / SPOT_LIGHT_SAMPLE_NUM as f32;
            for i in 0..SPOT_LIGHT_SAMPLE_NUM {
                let sample_dir = compute_fibonacci_spiral_disk_sample_uniform(
                    i,
                    sample_count_inverse,
                    SPOT_LIGHT_FIBONACCI_CLUMP_EXPONENT,
                    0.0,
                );
                let x = (pcf_radius as f32 * sample_dir.x).round() as i32;
                let y = (pcf_radius as f32 * sample_dir.y).round() as i32;
                let idx_x = clamp(center_x + x, 0, self.z_buffer_width - 1);
                let idx_y = clamp(center_y + y, 0, self.z_buffer_height - 1);
                if transform_pos.z + (max(x.abs(), y.abs()) + 1) as f32 * bias
                    > self.z_buffer[self.get_index(idx_x, idx_y) as usize]
                {
                    unshadow_num += 1;
                }
            }
            unshadow_num as f32 / SPOT_LIGHT_SAMPLE_NUM as f32
        } else {
            for y in -pcf_radius..=pcf_radius {
                for x in -pcf_radius..=pcf_radius {
                    let idx_x = clamp(center_x + x, 0, self.z_buffer_width - 1);
                    let idx_y = clamp(center_y + y, 0, self.z_buffer_height - 1);
                    if transform_pos.z + (max(x.abs(), y.abs()) + 1) as f32 * bias
                        > self.z_buffer[self.get_index(idx_x, idx_y) as usize]
                    {
                        unshadow_num += 1;
                    }
                }
            }
            unshadow_num as f32 / (2 * pcf_radius + 1).pow(2) as f32
        }
    }
    unsafe fn generate_penumbra_mask_block(
        &mut self,
        scene: *const crate::rasterization::scene::Scene,
        start_row: i32,
        start_col: i32,
        block_row: i32,
        block_col: i32,
    ) {
        for y in start_row..start_row + block_row {
            for x in start_col..start_col + block_col {
                let start_x = 4 * x;
                let start_y = 4 * y;
                let mut pcf_num = 0;
                let mut unshadow_num = 0;
                for v in start_y..start_y + 4 {
                    for u in start_x..start_x + 4 {
                        let idx = (*scene).get_index(
                            clamp(u, 0, (*scene).width - 1),
                            clamp(v, 0, (*scene).height - 1),
                        ) as usize;
                        if (*scene).z_buffer[idx] < INFINITY {
                            pcf_num += 1;
                            if self.in_shadow_direct(
                                (*scene).pos_buffer[idx],
                                (*scene).normal_buffer[idx],
                            ) > EPSILON
                            {
                                unshadow_num += 1;
                            }
                        }
                        let idx = self.get_penumbra_mask_index(x, y) as usize;
                        if unshadow_num == 0 || pcf_num == unshadow_num {
                            self.penumbra_mask[idx] = 0.0;
                        } else {
                            self.penumbra_mask[idx] = 1.0;
                        }
                    }
                }
            }
        }
    }
}
