use crate::rasterization::texture;
use crate::rasterization::triangle;
use std::rc::Rc;

use super::texture::Texture;
use super::triangle::*;
#[derive(Clone)]
pub struct Model {
    pub sub_models: Vec<Rc<Model>>,
    pub triangles: Vec<Triangle>,
    pub clip_triangles: Vec<TriangleRasterization>,
    pub pos: Vector3f,
    pub scale: f32,
    pub textures: [Option<Rc<Texture>>; Model::TEXTURE_NUM],
}
pub enum Textures {
    Diffuse = 0,
    Specular = 1,
    Normal = 2,
    Glow = 3,
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
}
