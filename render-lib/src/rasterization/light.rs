use crate::util::math::*;
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

impl Light {
    pub fn new(pos: Vector3f, intensity: Vector3f) -> Light {
        Light {
            pos: pos,
            intensity: intensity,
        }
    }
    pub fn set_pos(&mut self, pos: Vector3f) {
        self.pos = pos;
    }
    pub fn get_pos(&self) -> Vector3f {
        self.pos
    }
    pub fn set_intensity(&mut self, intensity: Vector3f) {
        self.intensity = intensity;
    }
    pub fn get_intensity(&self) -> Vector3f {
        self.intensity
    }
    pub fn look_at(scene: &crate::rasterization::scene::Scene) {}
    pub fn in_shadow(point: Vector3f, normal: Vector3f, shadow_method: ShadowMethod) -> f32 {
        1.0
    }
    pub fn in_penumbra_mask(x: i32, y: i32) -> bool {
        false
    }
    pub fn in_shadow_direct(point: Vector3f, normal: Vector3f) -> f32 {
        1.0
    }
    pub fn in_shadow_pcf(point: Vector3f, normal: Vector3f) -> f32 {
        1.0
    }
    pub fn in_shadow_pcss(point: Vector3f, normal: Vector3f) -> f32 {
        1.0
    }
    pub fn generate_penumbra_mask_block(
        scene: &crate::rasterization::scene::Scene,
        start_row: i32,
        start_col: i32,
        block_row: i32,
        block_col: i32,
    ) {
    }
    pub fn box_blur_penumbra_mask(radius: i32) {}
}
pub trait LightTrait {
    fn set_pos(&mut self, pos: Vector3f);
    fn get_pos(&self) -> Vector3f;
    fn set_intensity(&mut self, intensity: Vector3f);
    fn get_intensity(&self) -> Vector3f;
    fn look_at(&mut self, scene: &crate::rasterization::scene::Scene);
    fn in_shadow(&self, point: Vector3f, normal: Vector3f, shadow_method: ShadowMethod) -> f32;
    fn in_penumbra_mask(&self, x: i32, y: i32) -> bool;
    fn in_shadow_direct(&self, point: Vector3f, normal: Vector3f) -> f32;
    fn in_shadow_pcf(&self, point: Vector3f, normal: Vector3f) -> f32;
    fn in_shadow_pcss(&self, point: Vector3f, normal: Vector3f) -> f32;
    fn generate_penumbra_mask_block(
        &mut self,
        scene: &crate::rasterization::scene::Scene,
        start_row: i32,
        start_col: i32,
        block_row: i32,
        block_col: i32,
    );
    fn box_blur_penumbra_mask(&mut self, radius: i32);
}
