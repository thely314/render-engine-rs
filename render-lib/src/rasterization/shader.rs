use nalgebra::clamp;
use nalgebra::max;
use std::f32::INFINITY;

use crate::rasterization::light::*;
use crate::rasterization::scene::*;
use crate::util::math::Vector3f;
use crate::util::math::EPSILON;
pub unsafe fn default_texture_shader(
    scene: *mut Scene,
    lights: *const Vec<*const dyn LightTrait>,
    start_row: i32,
    start_col: i32,
    block_row: i32,
    block_col: i32,
) {
    let start_row = clamp(start_row, 0, (*scene).height - 1);
    let start_col = clamp(start_col, 0, (*scene).width - 1);
    let end_row = clamp(start_row + block_row, 0, (*scene).height);
    let end_col = clamp(start_col + block_col, 0, (*scene).width);
    for y in start_row..end_row {
        for x in start_col..end_col {
            let idx = (*scene).get_index(x, y) as usize;
            if (*scene).z_buffer[idx] == INFINITY {
                continue;
            }
            let point = (*scene).pos_buffer[idx];
            let normal = (*scene).normal_buffer[idx];
            let ka = (*scene).diffuse_buffer[idx];
            let kd = (*scene).diffuse_buffer[idx];
            let ks = (*scene).specular_buffer[idx];
            let mut result_color = (*scene).glow_buffer[idx];
            let ambient_intensity = Vector3f::new(0.05, 0.05, 0.05);
            for light in &(*lights) {
                let visiblity: f32;
                let light_ref = (*light).as_ref().unwrap();
                if light_ref.in_penumbra_mask(x, y) {
                    // result_color = Vector3f::new(1.0, 1.0, 1.0);
                    visiblity = light_ref.in_shadow(point, normal, ShadowMethod::PCSS);
                } else {
                    // result_color = Vector3f::new(0.0, 0.0, 0.0);
                    visiblity = light_ref.in_shadow(point, normal, ShadowMethod::DIRECT);
                }
                // break;
                let ambient = ka.component_mul(&ambient_intensity);
                result_color += ambient;
                if visiblity < EPSILON {
                    continue;
                }
                let light_pos: Vector3f;
                let light_intensity: Vector3f;
                light_pos = light_ref.get_pos();
                light_intensity = light_ref.get_intensity();
                let view_dir = ((*scene).eye_pos - point).normalize();
                let light_dir = light_pos - point;
                let distance_square = light_dir.dot(&light_dir);
                let light_dir = light_dir.normalize();
                let half_dir = (view_dir + light_dir).normalize();
                let diffuse = f32::max(0.0, light_dir.dot(&normal)) / distance_square
                    * kd.component_mul(&light_intensity);
                let specular = f32::max(0.0, half_dir.dot(&normal)).powf(150.0) / distance_square
                    * ks.component_mul(&light_intensity);
                result_color += visiblity * (diffuse + specular);
            }
            (*scene).frame_buffer[idx] = result_color;
        }
    }
}
