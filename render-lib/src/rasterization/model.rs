use assimp::Color4D;
use assimp::Vector3D;

use crate::rasterization::scene::*;
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
pub enum TextureTypes {
    Diffuse = 0,
    Specular = 1,
    Normal = 2,
    Glow = 3,
}
impl TextureTypes {
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
    pub fn from_file(path: &str, default_color: Color4D) -> Self {
        let mut importer = assimp::Importer::new();
        importer.triangulate(true);
        importer.generate_normals(|config: &mut assimp::import::structs::GenerateNormals| {
            // 启用法线生成 (通常在此闭包中默认为启用，但显式设置更清晰)
            config.enable = true;
            // 启用法线平滑
            config.smooth = true;
            // 设置最大平滑角度。Assimp 的默认值通常在 60-88 度之间。
            // 66.0 是一个常见选择。
            config.max_smoothing_angle = 66.0; // 单位是度
                                               // 根据 crate 文档，GenerateNormals 的其他字段（如果有的话）将使用其默认值。
                                               // 例如，通常不强制覆盖已存在的法线，除非有特定字段并被设置为 true。
        });
        let mut output = Self::default();
        match importer.read_file(path) {
            Ok(scene) => {
                println!("成功加载模型: {}", path);
                for mesh in scene.mesh_iter() {
                    output.process_mesh(&mesh, default_color);
                }
            }
            Err(err) => {
                eprintln!("加载模型失败: {}", err);
            }
        }
        output
    }
    pub fn load(&mut self, path: &str, default_color: Color4D) {
        let mut importer = assimp::Importer::new();
        importer.triangulate(true);
        importer.generate_normals(|config: &mut assimp::import::structs::GenerateNormals| {
            config.enable = true;
            config.smooth = true;
            config.max_smoothing_angle = 66.0;
        });
        match importer.read_file(path) {
            Ok(scene) => {
                println!("成功加载模型: {}", path);
                self.triangles.clear();
                self.clip_triangles.clear();
                self.pos = Vector3f::new(0.0, 0.0, 0.0);
                self.scale = 1.0;
                for mesh in scene.mesh_iter() {
                    self.process_mesh(&mesh, default_color);
                }
            }
            Err(err) => {
                eprintln!("加载模型失败: {}", err);
            }
        }
    }
    pub fn add_sub_model(&mut self, sub_model: Arc<Mutex<Model>>) {
        self.sub_models.push(sub_model);
    }
    fn process_mesh(&mut self, mesh: &assimp::Mesh, default_color: Color4D) {
        let mut vertices = Vec::new();
        for i in 0..mesh.num_vertices() {
            let position = mesh.get_vertex(i).unwrap_or(Vector3D::new(0.0, 0.0, 0.0));
            let normal = mesh.get_normal(i).unwrap_or(Vector3D::new(0.0, 0.0, 0.0));
            let tex_coords = if mesh.has_texture_coords(0) {
                mesh.get_texture_coord(0, i)
                    .unwrap_or(Vector3D::new(0.0, 0.0, 0.0))
            } else {
                Vector3D::new(0.0, 0.0, 0.0)
            };
            let color = if mesh.has_vertex_colors(0) {
                mesh.get_vertex_color(0, i).unwrap_or(default_color)
            } else {
                default_color
            };
            let vertex = Vertex {
                pos: Vector3f::new(position.x, position.y, position.z),
                normal: Vector3f::new(normal.x, normal.y, normal.z),
                texture_coords: Vector2f::new(tex_coords.x, tex_coords.y),
                color: Vector3f::new(color.r, color.g, color.b),
            };
            vertices.push(vertex);
        }

        for face in mesh.face_iter() {
            if face.num_indices == 3 {
                let indices =
                    unsafe { std::slice::from_raw_parts(face.indices, face.num_indices as usize) };
                self.triangles.push(Triangle::new(
                    &vertices[indices[0] as usize],
                    &vertices[indices[1] as usize],
                    &vertices[indices[2] as usize],
                ));
            }
        }
    }

    pub fn set_pos(&mut self, pos: Vector3f) {
        let modeling_matrix = Matrix4f::from_row_slice(&[
            1.0,
            0.0,
            0.0,
            pos.x - self.pos.x,
            0.0,
            1.0,
            0.0,
            pos.y - self.pos.y,
            0.0,
            0.0,
            1.0,
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
    pub fn rotate(&mut self, axis: Vector3f, angle: f32) {
        let axis = axis.normalize();
        let mut move_to_origin = Matrix4f::identity();
        let mut rotate = get_modeling_matrix(axis, angle, Vector3f::new(0.0, 0.0, 0.0));
        let mut move_to_pos = Matrix4f::identity();
        move_to_origin[(0, 3)] = -self.pos.x;
        move_to_origin[(1, 3)] = -self.pos.y;
        move_to_origin[(2, 3)] = -self.pos.z;
        move_to_pos[(0, 3)] = self.pos.x;
        move_to_pos[(1, 3)] = self.pos.y;
        move_to_pos[(2, 3)] = self.pos.z;
        rotate = move_to_pos * rotate * move_to_origin;
        for sub_model in &self.sub_models {
            sub_model.lock().unwrap().modeling(&rotate);
        }
        for triangle in &mut self.triangles {
            triangle.modeling(&rotate);
        }
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
    pub fn set_texture(&mut self, texture: Option<Arc<Texture>>, id: TextureTypes) {
        self.textures[id.as_usize()] = texture;
    }
    pub fn get_texture(&self, id: TextureTypes) -> Option<Arc<Texture>> {
        self.textures[id.as_usize()].clone()
    }
    pub(in crate::rasterization) unsafe fn rasterization(
        &self,
        scene: *mut Scene,
        start_row: i32,
        start_col: i32,
        block_row: i32,
        block_col: i32,
    ) {
        for sub_model in &self.sub_models {
            sub_model
                .lock()
                .unwrap()
                .rasterization(scene, start_row, start_col, block_row, block_col);
        }
        for triangle in &self.clip_triangles {
            triangle.rasterization(scene, &self, start_row, start_col, block_row, block_col);
        }
    }
    pub(in crate::rasterization) unsafe fn rasterization_shadow_map<const IS_PROJECTION: bool>(
        &self,
        z_buffer: *mut Vec<f32>,
        start_row: i32,
        start_col: i32,
        block_row: i32,
        block_col: i32,
        depth_transformer: &dyn Fn(f32, f32) -> f32,
        get_index: &dyn Fn(i32, i32) -> i32,
    ) {
        for sub_model in &self.sub_models {
            sub_model
                .lock()
                .unwrap()
                .rasterization_shadow_map::<IS_PROJECTION>(
                    z_buffer,
                    start_row,
                    start_col,
                    block_row,
                    block_col,
                    &depth_transformer,
                    &get_index,
                );
        }
        for triangle in &self.clip_triangles {
            triangle.rasterization_shadow_map::<IS_PROJECTION>(
                z_buffer,
                start_row,
                start_col,
                block_row,
                block_col,
                &depth_transformer,
                &get_index,
            );
        }
    }
    pub fn modeling(&mut self, modeling_matrix: &Matrix4f) {
        for sub_model in &mut self.sub_models {
            sub_model.lock().unwrap().modeling(modeling_matrix);
        }
        for triangle in &mut self.triangles {
            triangle.modeling(modeling_matrix);
        }
    }
    pub fn to_NDC(&mut self, width: u32, height: u32) {
        for sub_model in &mut self.sub_models {
            sub_model.lock().unwrap().to_NDC(width, height);
        }
        for triangle in &mut self.clip_triangles {
            triangle.to_NDC(width, height);
        }
    }
    pub fn clip(&mut self, mvp: &Matrix4f, mv: &Matrix4f) {
        self.clip_triangles.clear();
        for sub_model in &mut self.sub_models {
            sub_model.lock().unwrap().clip(mvp, mv);
        }
        for triangle in &mut self.triangles {
            triangle.clip(mvp, mv, &mut self.clip_triangles);
        }
    }
}
