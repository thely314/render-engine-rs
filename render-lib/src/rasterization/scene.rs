use std::f32::INFINITY;
use std::sync::{Arc, Mutex, RwLock};
use std::thread;

use nalgebra::{max, min};

use crate::rasterization::light::*;
use crate::rasterization::model::*;
use crate::util::math::*;
const SCENE_MAXIMUM_THREAD_NUM: i32 = 8;
#[derive(Copy, Clone)]
struct ScenePtr(*mut Scene);
unsafe impl Send for ScenePtr {}
impl ScenePtr {
    unsafe fn get_mut(self) -> *mut Scene {
        self.0
    }
}
pub struct Scene {
    pub(in crate::rasterization) eye_pos: Vector3f,
    pub(in crate::rasterization) view_dir: Vector3f,
    pub(in crate::rasterization) fov: f32,
    pub(in crate::rasterization) aspect_ratio: f32,
    pub(in crate::rasterization) z_near: f32,
    pub(in crate::rasterization) z_far: f32,
    pub(in crate::rasterization) width: i32,
    pub(in crate::rasterization) height: i32,
    pub(in crate::rasterization) frame_buffer: Vec<Vector3f>,
    pub(in crate::rasterization) pos_buffer: Vec<Vector3f>,
    pub(in crate::rasterization) normal_buffer: Vec<Vector3f>,
    pub(in crate::rasterization) diffuse_buffer: Vec<Vector3f>,
    pub(in crate::rasterization) specular_buffer: Vec<Vector3f>,
    pub(in crate::rasterization) glow_buffer: Vec<Vector3f>,
    pub(in crate::rasterization) z_buffer: Vec<f32>,
    pub(in crate::rasterization) models: Vec<Arc<RwLock<Model>>>,
    pub(in crate::rasterization) lights: Vec<Arc<RwLock<dyn LightTrait>>>,
    pub(in crate::rasterization) shader: unsafe fn(*mut Scene, i32, i32, i32, i32),
}
impl Default for Scene {
    fn default() -> Self {
        Scene {
            eye_pos: Vector3f::new(0.0, 0.0, 0.0),
            view_dir: Vector3f::new(0.0, 0.0, -1.0),
            fov: 45.0,
            aspect_ratio: 1.0,
            z_near: -0.1,
            z_far: -100.0,
            width: 1024,
            height: 1024,
            frame_buffer: Vec::default(),
            pos_buffer: Vec::default(),
            normal_buffer: Vec::default(),
            diffuse_buffer: Vec::default(),
            specular_buffer: Vec::default(),
            glow_buffer: Vec::default(),
            z_buffer: Vec::default(),
            models: Vec::default(),
            lights: Vec::default(),
            shader: crate::rasterization::shader::default_texture_shader,
        }
    }
}
impl Scene {
    pub fn new(width: i32, height: i32) -> Self {
        Scene {
            eye_pos: Vector3f::new(0.0, 0.0, 0.0),
            view_dir: Vector3f::new(0.0, 0.0, -1.0),
            fov: 45.0,
            aspect_ratio: 1.0,
            z_near: -0.1,
            z_far: -100.0,
            width: width,
            height: height,
            frame_buffer: Vec::default(),
            pos_buffer: Vec::default(),
            normal_buffer: Vec::default(),
            diffuse_buffer: Vec::default(),
            specular_buffer: Vec::default(),
            glow_buffer: Vec::default(),
            z_buffer: Vec::default(),
            models: Vec::default(),
            lights: Vec::default(),
            shader: crate::rasterization::shader::default_texture_shader,
        }
    }
    pub fn add_model(&mut self, model: Arc<RwLock<Model>>) {
        self.models.push(model);
    }
    pub fn add_light(&mut self, light: Arc<RwLock<dyn LightTrait>>) {
        self.lights.push(light);
    }
    pub fn delete_model(&mut self, target: &Arc<RwLock<Model>>) {
        self.models.retain(|model| !Arc::ptr_eq(model, target));
    }

    pub fn delete_light(&mut self, target: &Arc<RwLock<dyn LightTrait>>) {
        self.lights.retain(|light| !Arc::ptr_eq(light, target));
    }
    pub fn get_index(&self, x: i32, y: i32) -> i32 {
        self.width * y + x
    }
    pub fn get_penumbra_mask_index(&self, x: i32, y: i32) -> i32 {
        self.width * (y / 4) + (x / 4)
    }
    pub fn set_eye_pos(&mut self, eye_pos: Vector3f) {
        self.eye_pos = eye_pos;
    }
    pub fn get_eye_pos(&self) -> Vector3f {
        self.eye_pos
    }
    pub fn set_view_dir(&mut self, view_dir: Vector3f) {
        self.view_dir = view_dir.normalize();
    }
    pub fn get_view_dir(&self) -> Vector3f {
        self.view_dir
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
    pub fn set_width(&mut self, width: i32) {
        self.width = width;
    }
    pub fn get_width(&self) -> i32 {
        self.width
    }
    pub fn set_height(&mut self, height: i32) {
        self.height = height;
    }
    pub fn get_height(&self) -> i32 {
        self.height
    }
    pub fn set_shader(&mut self, shader: unsafe fn(*mut Scene, i32, i32, i32, i32)) {
        self.shader = shader;
    }
    pub fn get_shader(&self) -> unsafe fn(*mut Scene, i32, i32, i32, i32) {
        self.shader
    }
    //   void start_render();
    pub fn start_render(&mut self) {
        self.frame_buffer.resize(
            (self.width * self.height) as usize,
            Vector3f::new(0.0, 0.0, 0.0),
        );
        self.pos_buffer.resize(
            (self.width * self.height) as usize,
            Vector3f::new(0.0, 0.0, 0.0),
        );
        self.normal_buffer.resize(
            (self.width * self.height) as usize,
            Vector3f::new(0.0, 0.0, 0.0),
        );
        self.diffuse_buffer.resize(
            (self.width * self.height) as usize,
            Vector3f::new(0.0, 0.0, 0.0),
        );
        self.specular_buffer.resize(
            (self.width * self.height) as usize,
            Vector3f::new(0.0, 0.0, 0.0),
        );
        self.glow_buffer.resize(
            (self.width * self.height) as usize,
            Vector3f::new(0.0, 0.0, 0.0),
        );
        self.z_buffer
            .resize((self.width * self.height) as usize, INFINITY);
        self.frame_buffer.fill(Vector3f::new(0.7, 0.7, 0.7));
        self.pos_buffer.fill(Vector3f::new(0.0, 0.0, 0.0));
        self.normal_buffer.fill(Vector3f::new(0.0, 0.0, 0.0));
        self.diffuse_buffer.fill(Vector3f::new(0.0, 0.0, 0.0));
        self.specular_buffer.fill(Vector3f::new(0.0, 0.0, 0.0));
        self.glow_buffer.fill(Vector3f::new(0.0, 0.0, 0.0));
        self.z_buffer.fill(INFINITY);
        let model = Matrix4f::identity();
        let view = get_view_matrix(self.eye_pos, self.view_dir);
        let projection =
            get_projection_matrix(self.fov, self.aspect_ratio, self.z_near, self.z_far);
        let mvp = projection * view * model;
        let mv = view * model;
        {
            let mut locked_lights = vec![];
            for light in &self.lights {
                locked_lights.push(light.write().unwrap());
            }
            for light in &mut locked_lights {
                light.look_at(&self);
            }
            let mut locked_models = vec![];
            for model in &self.models {
                locked_models.push(model.write().unwrap());
            }
            for model in &mut locked_models {
                model.clip(&mvp, &mv);
                model.to_NDC(self.width as u32, self.height as u32);
            }
        }
        let thread_num = std::cmp::min(self.height, SCENE_MAXIMUM_THREAD_NUM);
        let rows_per_thread = (self.height as f32 / thread_num as f32).ceil() as i32;
        let scene_ptr = ScenePtr(self as *mut Scene);
        let self_width = self.width;
        let self_height = self.height;
        let mut handles = Vec::new();
        for tid in 0..thread_num - 1 {
            let handle = thread::spawn(move || unsafe {
                let scene: *mut Scene = scene_ptr.get_mut();
                for model in &(*scene).models {
                    let model = model.read().unwrap();
                    model.rasterization(
                        scene,
                        tid * rows_per_thread,
                        0,
                        rows_per_thread,
                        self_width,
                    );
                }
            });
            handles.push(handle);
        }
        let handle = thread::spawn(move || unsafe {
            let scene: *mut Scene = scene_ptr.get_mut();
            for model in &(*scene).models {
                let model = model.read().unwrap();
                model.rasterization(
                    scene,
                    (thread_num - 1) * rows_per_thread,
                    0,
                    self_height - (thread_num - 1) * rows_per_thread,
                    self_width,
                );
            }
        });
        handles.push(handle);
        for handle in handles {
            handle.join().unwrap();
        }
        let box_radius = (4.0 * max(self.width, self.height) as f32 / 1024.0).round() as i32;
        let penumbra_width = (self.width as f32 * 0.25).ceil() as i32;
        let penumbra_height = (self.height as f32 * 0.25).ceil() as i32;
        let penumbra_mask_thread_num = min(penumbra_height, SCENE_MAXIMUM_THREAD_NUM);
        let penumbra_rows_per_thread =
            (penumbra_height as f32 / penumbra_mask_thread_num as f32).ceil() as i32;
        //   auto penumbra_mask_lambda = [](Scene &scene, int start_row, int block_row) {
        //     for (auto &&light : scene.lights) {
        //       light->generate_penumbra_mask_block(scene, start_row, 0, block_row,
        //                                           ceilf(scene.width * 0.25f));
        //     }
        //   };
        //   for (int i = 0; i < penumbra_mask_thread_num - 1; ++i) {
        //     threads.emplace_back(penumbra_mask_lambda, std::ref(*this),
        //                          penumbra_mask_thread_row_num * i,
        //                          penumbra_mask_thread_row_num);
        //   }
        //   threads.emplace_back(penumbra_mask_lambda, std::ref(*this),
        //                        penumbra_mask_thread_row_num * (thread_num - 1),
        //                        penumbra_height - penumbra_mask_thread_row_num *
        //                                              (penumbra_mask_thread_num - 1));
        //   for (auto &&thread : threads) {
        //     thread.join();
        //   }
        //   threads.clear();
    }
    //   void save_to_file(std::string filename);
}
