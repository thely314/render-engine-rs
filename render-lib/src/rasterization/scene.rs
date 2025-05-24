use std::sync::{Arc, Mutex};

use crate::rasterization::light::*;
use crate::rasterization::model::*;
use crate::util::math::*;

pub struct scene {
    pub(in crate::rasterization) eye_pos: Vector3f,
    pub(in crate::rasterization) view_dir: Vector3f,
    pub(in crate::rasterization) zNear: f32,
    pub(in crate::rasterization) zFar: f32,
    pub(in crate::rasterization) width: i32,
    pub(in crate::rasterization) height: i32,
    pub(in crate::rasterization) frame_buffer: Vec<Vector3f>,
    pub(in crate::rasterization) normal_buffer: Vec<Vector3f>,
    pub(in crate::rasterization) diffuse_buffer: Vec<Vector3f>,
    pub(in crate::rasterization) specular_buffer: Vec<Vector3f>,
    pub(in crate::rasterization) glow_buffer: Vec<Vector3f>,
    pub(in crate::rasterization) z_buffer: Vec<f32>,
    pub(in crate::rasterization) models: Vec<Arc<Mutex<Model>>>,
    pub(in crate::rasterization) lights: Vec<Arc<dyn Light>>,
}
impl Default for scene {
    fn default() -> Self {
        scene {
            eye_pos: Vector3f::new(0.0, 0.0, 0.0),
            view_dir: Vector3f::new(0.0, 0.0, -1.0),
            zNear: -0.1,
            zFar: -100.0,
            width: 1024,
            height: 1024,
            frame_buffer: Vec::default(),
            normal_buffer: Vec::default(),
            diffuse_buffer: Vec::default(),
            specular_buffer: Vec::default(),
            glow_buffer: Vec::default(),
            z_buffer: Vec::default(),
            models: Vec::default(),
            lights: Vec::default(),
        }
    }
}
