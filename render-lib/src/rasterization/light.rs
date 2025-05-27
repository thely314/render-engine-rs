use parking_lot::RwLockWriteGuard;

use crate::util::math::*;

use super::model::Model;
#[derive(Clone)]
pub struct Light {
    pub(in crate::rasterization) pos: Vector3f,
    pub(in crate::rasterization) intensity: Vector3f,
}
pub enum ShadowMethod {
    DIRECT,
    PCF,
    PCSS,
}
impl Default for Light {
    fn default() -> Self {
        Light {
            pos: Vector3f::new(0.0, 0.0, 0.0),
            intensity: Vector3f::new(0.0, 0.0, 0.0),
        }
    }
}
unsafe impl Send for Light {}
unsafe impl Sync for Light {}
impl LightTrait for Light {
    fn set_pos(&mut self, pos: Vector3f) {
        self.pos = pos;
    }
    fn get_pos(&self) -> Vector3f {
        self.pos
    }
    fn set_intensity(&mut self, intensity: Vector3f) {
        self.intensity = intensity;
    }
    fn get_intensity(&self) -> Vector3f {
        self.intensity
    }
    fn look_at(&mut self, models: &mut Vec<RwLockWriteGuard<Model>>) {}
    fn in_shadow(&self, point: Vector3f, normal: Vector3f, shadow_method: ShadowMethod) -> f32 {
        1.0
    }
    fn in_penumbra_mask(&self, x: i32, y: i32) -> bool {
        false
    }
    fn generate_penumbra_mask(&mut self, scene: *const crate::rasterization::scene::Scene) {

        // let penumbra_width = (self.width as f32 * 0.25).ceil() as i32;
        // let penumbra_height = (self.height as f32 * 0.25).ceil() as i32;
        // let penumbra_mask_thread_num = min(penumbra_height, SCENE_MAXIMUM_THREAD_NUM);
        // let penumbra_rows_per_thread =
        //     (penumbra_height as f32 / penumbra_mask_thread_num as f32).ceil() as i32;
    }
    fn box_blur_penumbra_mask(&mut self, radius: i32) {}
}
impl Light {
    pub fn new(pos: Vector3f, intensity: Vector3f) -> Light {
        Light {
            pos: pos,
            intensity: intensity,
        }
    }

    pub fn in_shadow_direct(&self, point: Vector3f, normal: Vector3f) -> f32 {
        1.0
    }
    pub fn in_shadow_pcf(&self, point: Vector3f, normal: Vector3f) -> f32 {
        1.0
    }
    pub fn in_shadow_pcss(&self, point: Vector3f, normal: Vector3f) -> f32 {
        1.0
    }
    pub fn generate_penumbra_mask(&mut self, scene: *const crate::rasterization::scene::Scene) {

        // let penumbra_width = (self.width as f32 * 0.25).ceil() as i32;
        // let penumbra_height = (self.height as f32 * 0.25).ceil() as i32;
        // let penumbra_mask_thread_num = min(penumbra_height, SCENE_MAXIMUM_THREAD_NUM);
        // let penumbra_rows_per_thread =
        //     (penumbra_height as f32 / penumbra_mask_thread_num as f32).ceil() as i32;
    }
    fn generate_penumbra_mask_block(
        &mut self,
        scene: *const crate::rasterization::scene::Scene,
        start_row: i32,
        start_col: i32,
        block_row: i32,
        block_col: i32,
    ) {
    }
}
pub trait LightTrait: Send + Sync {
    fn set_pos(&mut self, pos: Vector3f);
    fn get_pos(&self) -> Vector3f;
    fn set_intensity(&mut self, intensity: Vector3f);
    fn get_intensity(&self) -> Vector3f;
    fn look_at(&mut self, models: &mut Vec<RwLockWriteGuard<Model>>);
    fn in_shadow(&self, point: Vector3f, normal: Vector3f, shadow_method: ShadowMethod) -> f32;
    fn in_penumbra_mask(&self, x: i32, y: i32) -> bool;
    fn generate_penumbra_mask(&mut self, scene: *const crate::rasterization::scene::Scene);
    fn box_blur_penumbra_mask(&mut self, radius: i32);
}
