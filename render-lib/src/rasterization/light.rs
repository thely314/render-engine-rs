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
    fn compute_world_light_dir(&self, point: Vector3f) -> Vector3f {
        (point - self.pos).normalize()
    }
    fn compute_world_light_intensity(&self, point: Vector3f) -> Vector3f {
        self.intensity / ((point - self.pos).dot(&(point - self.pos)))
    }
    fn look_at(&mut self, models: *const Vec<*mut Model>) {}
    fn in_shadow(&self, point: Vector3f, normal: Vector3f, shadow_method: ShadowMethod) -> f32 {
        1.0
    }
    fn in_penumbra_mask(&self, x: i32, y: i32) -> bool {
        false
    }
    fn generate_penumbra_mask(&mut self, scene: *const crate::rasterization::scene::Scene) {}
    fn box_blur_penumbra_mask(&mut self, radius: i32) {}
}
impl Light {
    pub fn new(pos: Vector3f, intensity: Vector3f) -> Light {
        Light {
            pos: pos,
            intensity: intensity,
        }
    }

    pub(in crate::rasterization) fn in_shadow_direct(
        &self,
        point: Vector3f,
        normal: Vector3f,
    ) -> f32 {
        1.0
    }
    pub(in crate::rasterization) fn in_shadow_pcf(&self, point: Vector3f, normal: Vector3f) -> f32 {
        1.0
    }
    pub(in crate::rasterization) fn in_shadow_pcss(
        &self,
        point: Vector3f,
        normal: Vector3f,
    ) -> f32 {
        1.0
    }
    pub(in crate::rasterization) fn generate_penumbra_mask(
        &mut self,
        scene: *const crate::rasterization::scene::Scene,
    ) {
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
    /// 计算世界空间下的光照方向
    fn compute_world_light_dir(&self, point: Vector3f) -> Vector3f;
    /// 计算世界空间下的光照强度
    fn compute_world_light_intensity(&self, point: Vector3f) -> Vector3f;
    /// 以models为依照生成shadow map
    fn look_at(&mut self, models: *const Vec<*mut Model>);
    /// 探测某个点是否在阴影中
    fn in_shadow(&self, point: Vector3f, normal: Vector3f, shadow_method: ShadowMethod) -> f32;
    /// 探测某个点是否在penumbra_mask中
    /// penumbra_mask先生成一遍硬阴影，找到阴影边界，然后对阴影边界做盒式滤波模糊
    /// 只有在“阴影边界”范围内的点才会被判断为在penumbra_mask中，需要做软阴影计算
    fn in_penumbra_mask(&self, x: i32, y: i32) -> bool;
    /// 生成penumbra_mask
    fn generate_penumbra_mask(&mut self, scene: *const crate::rasterization::scene::Scene);
    /// 模糊penumbra_mask
    fn box_blur_penumbra_mask(&mut self, radius: i32);
}
