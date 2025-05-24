use crate::rasterization::texture::*;
use crate::rasterization::triangle::*;
use crate::util::math::*;
use std::sync::Arc;
use std::sync::Mutex;
#[derive(Clone)]
pub struct Model {
    sub_models: Vec<Arc<Mutex<Model>>>,
    triangles: Vec<Triangle>,
    clip_triangles: Vec<TriangleRasterization>,
    pos: Vector3f,
    scale: f32,
    textures: [Option<Arc<Texture>>; Model::TEXTURE_NUM],
}
#[derive(Copy, Clone)]
pub enum Textures {
    Diffuse = 0,
    Specular = 1,
    Normal = 2,
    Glow = 3,
}
impl Textures {
    pub const fn as_usize(&self) -> usize {
        *self as usize
    }
}
impl Default for Model {
    fn default() -> Self {
        Model {
            sub_models: Vec::default(),
            triangles: Vec::default(),
            clip_triangles: Vec::default(),
            pos: Vector3f::new(0.0, 0.0, 0.0),
            scale: 1.0,
            textures: std::array::from_fn(|_| None),
        }
    }
}

impl Model {
    pub const TEXTURE_NUM: usize = 4;
    pub fn set_pos(&mut self, pos: Vector3f) {
        let modeling_matrix = Matrix4f::from_row_slice(&[
            0.0,
            0.0,
            0.0,
            pos.x - self.pos.x,
            0.0,
            0.0,
            0.0,
            pos.y - self.pos.y,
            0.0,
            0.0,
            0.0,
            pos.z - self.pos.z,
            0.0,
            0.0,
            0.0,
            1.0,
        ]);
        self.modeling(&modeling_matrix);
        self.pos = pos;
    }
    pub fn get_pos(&self) -> Vector3f {
        self.pos
    }
    pub fn set_scale(&mut self, scale: f32) {
        let scale_rate = scale / self.scale;
        let scale_matrix = Matrix4f::from_row_slice(&[
            scale_rate, 0.0, 0.0, 0.0, 0.0, scale_rate, 0.0, 0.0, 0.0, 0.0, scale_rate, 0.0, 0.0,
            0.0, 0.0, 1.0,
        ]);
        self.modeling(&scale_matrix);
        self.scale = scale;
    }
    pub fn get_scale(&self) -> f32 {
        self.scale
    }
    pub fn set_texture(&mut self, texture: Option<Arc<Texture>>, id: Textures) {
        self.textures[id.as_usize()] = texture;
    }
    pub fn get_texture(&self, id: Textures) -> Option<Arc<Texture>> {
        self.textures[id.as_usize()].clone()
    }
    pub fn modeling(&mut self, modeling_matrix: &Matrix4f) {
        for sub_model in &self.sub_models {
            sub_model.lock().unwrap().modeling(modeling_matrix);
        }
        for triangle in &mut self.triangles {
            triangle.modeling(modeling_matrix);
        }
    }
    pub fn to_NDC(&mut self, width: u32, height: u32) {
        for sub_model in &self.sub_models {
            sub_model.lock().unwrap().to_NDC(width, height);
        }
        for triangle in &mut self.clip_triangles {
            triangle.to_NDC(width, height);
        }
    }
    pub fn clip(&mut self, mvp: &Matrix4f, mv: &Matrix4f) {
        for sub_model in &self.sub_models {
            sub_model.lock().unwrap().clip(mvp, mv);
        }
        for triangle in &mut self.triangles {
            triangle.clip(mvp, mv, &mut self.clip_triangles);
        }
    }
}
