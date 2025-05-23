use crate::rasterization::texture;
use crate::rasterization::triangle;
pub struct model {}
pub enum Textures {
    Diffuse = 0,
    Specular = 1,
    Normal = 2,
    Glow = 3,
}
impl model {
    pub const TEXTURE_NUM: usize = 4;
}
