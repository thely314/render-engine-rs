use nalgebra::{clamp, max, min};

use crate::rasterization;
use crate::util::math::*;

use super::model::*;
use super::scene::Scene;

#[derive(Debug)]
pub struct Vertex {
    pub pos: Vector3f,
    pub normal: Vector3f,
    pub color: Vector3f,
    pub texture_coords: Vector2f,
}

impl Default for Vertex {
    fn default() -> Self {
        Vertex {
            pos: Vector3f::new(0.0, 0.0, 0.0),
            normal: Vector3f::new(0.0, 0.0, 0.0),
            color: Vector3f::new(0.0, 0.0, 0.0),
            texture_coords: Vector2f::new(0.0, 0.0),
        }
    }
}

impl Clone for Vertex {
    fn clone(&self) -> Self {
        Vertex::new(self.pos, self.normal, self.color, self.texture_coords)
    }
}

impl Copy for Vertex {}

impl std::ops::Add<&Vertex> for Vertex {
    type Output = Vertex;
    fn add(self, other: &Vertex) -> Self {
        Vertex::new(
            self.pos + other.pos,
            (self.normal + other.normal).normalize(),
            self.color + other.color,
            self.texture_coords + other.texture_coords,
        )
    }
}

impl std::ops::Mul<f32> for Vertex {
    type Output = Vertex;
    fn mul(self, rhs: f32) -> Self {
        Vertex::new(
            rhs * self.pos,
            rhs * self.normal,
            rhs * self.color,
            rhs * self.texture_coords,
        )
    }
}

impl std::ops::Mul<&Vertex> for f32 {
    type Output = Vertex;
    fn mul(self, rhs: &Vertex) -> Vertex {
        Vertex::new(
            self * rhs.pos,
            self * rhs.normal,
            self * rhs.color,
            self * rhs.texture_coords,
        )
    }
}

impl Vertex {
    pub fn new(
        pos: Vector3f,
        normal: Vector3f,
        color: Vector3f,
        texture_coords: Vector2f,
    ) -> Vertex {
        Vertex {
            pos: pos,
            normal: normal,
            color: color,
            texture_coords: texture_coords,
        }
    }
}
#[derive(Debug)]
pub struct Triangle {
    pub verteies: [Vertex; 3],
}

impl Default for Triangle {
    fn default() -> Self {
        Triangle {
            verteies: [Vertex::default(); 3],
        }
    }
}

impl Clone for Triangle {
    fn clone(&self) -> Self {
        Triangle {
            verteies: [self.verteies[0], self.verteies[1], self.verteies[2]],
        }
    }
}

impl Copy for Triangle {}

impl Triangle {
    pub fn new(v0: &Vertex, v1: &Vertex, v2: &Vertex) -> Self {
        Triangle {
            verteies: [*v0, *v1, *v2],
        }
    }
    pub(in crate::rasterization) fn clip(
        &self,
        mvp: &Matrix4f,
        mv: &Matrix4f,
        output: &mut Vec<TriangleRasterization>,
    ) {
        let mut mv_pos: [Vector3f; 3] = [Vector3f::default(); 3];
        for i in 0..3 {
            mv_pos[i] = (mv * (homogeneous(self.verteies[i].pos))).xyz();
        }
        let mv_normal = (mv_pos[1] - mv_pos[0]).cross(&(mv_pos[2] - mv_pos[1]));
        for i in 0..3 {
            if mv_normal.dot(&mv_pos[i]) > EPSILON {
                return;
            }
        }
        let mut clip_triangles = vec![TriangleRasterization::from_triangle(self)];
        for i in 0..3 {
            clip_triangles[0].verteies[i].transform_pos =
                mvp * homogeneous(clip_triangles[0].verteies[i].pos);
        }
        TriangleRasterization::clip_triangles::<2, true>(&mut clip_triangles);
        TriangleRasterization::clip_triangles::<2, false>(&mut clip_triangles);
        TriangleRasterization::clip_triangles::<1, true>(&mut clip_triangles);
        TriangleRasterization::clip_triangles::<1, false>(&mut clip_triangles);
        TriangleRasterization::clip_triangles::<0, true>(&mut clip_triangles);
        TriangleRasterization::clip_triangles::<0, false>(&mut clip_triangles);
        // println!("Start");
        for iter in clip_triangles.into_iter() {
            // println!("{:?}\n", iter);
            output.push(iter);
        }
        // println!("End\n");
    }
    pub fn modeling(&mut self, matrix: &Matrix4f) {
        for i in 0..3 {
            self.verteies[i].pos = (matrix * homogeneous(self.verteies[i].pos)).xyz();
            self.verteies[i].normal = matrix.fixed_view::<3, 3>(0, 0) * self.verteies[i].normal;
        }
    }
}

#[derive(Debug)]
pub struct VertexRasterization {
    pub pos: Vector3f,
    pub transform_pos: Vector4f,
    pub normal: Vector3f,
    pub color: Vector3f,
    pub texture_coords: Vector2f,
}

impl Default for VertexRasterization {
    fn default() -> Self {
        VertexRasterization {
            pos: Vector3f::new(0.0, 0.0, 0.0),
            normal: Vector3f::new(0.0, 0.0, 0.0),
            transform_pos: Vector4f::new(0.0, 0.0, 0.0, 0.0),
            color: Vector3f::new(0.0, 0.0, 0.0),
            texture_coords: Vector2f::new(0.0, 0.0),
        }
    }
}
impl Clone for VertexRasterization {
    fn clone(&self) -> Self {
        VertexRasterization::new(
            self.pos,
            self.transform_pos,
            self.normal,
            self.color,
            self.texture_coords,
        )
    }
}

impl Copy for VertexRasterization {}

impl std::ops::Add<&VertexRasterization> for VertexRasterization {
    type Output = VertexRasterization;
    fn add(self, other: &VertexRasterization) -> Self {
        VertexRasterization::new(
            self.pos + other.pos,
            self.transform_pos + other.transform_pos,
            (self.normal + other.normal).normalize(),
            self.color + other.color,
            self.texture_coords + other.texture_coords,
        )
    }
}

impl std::ops::Mul<f32> for VertexRasterization {
    type Output = VertexRasterization;
    fn mul(self, rhs: f32) -> Self {
        VertexRasterization::new(
            rhs * self.pos,
            rhs * self.transform_pos,
            rhs * self.normal,
            rhs * self.color,
            rhs * self.texture_coords,
        )
    }
}

impl std::ops::Mul<&VertexRasterization> for f32 {
    type Output = VertexRasterization;
    fn mul(self, rhs: &VertexRasterization) -> VertexRasterization {
        VertexRasterization::new(
            self * rhs.pos,
            self * rhs.transform_pos,
            self * rhs.normal,
            self * rhs.color,
            self * rhs.texture_coords,
        )
    }
}

impl VertexRasterization {
    pub fn new(
        pos: Vector3f,
        transform_pos: Vector4f,
        normal: Vector3f,
        color: Vector3f,
        texture_coords: Vector2f,
    ) -> VertexRasterization {
        VertexRasterization {
            pos: pos,
            normal: normal,
            transform_pos: transform_pos,
            color: color,
            texture_coords: texture_coords,
        }
    }
    pub fn from_vertex(vertex: &Vertex, transform_pos: Vector4f) -> Self {
        VertexRasterization::new(
            vertex.pos,
            transform_pos,
            vertex.normal,
            vertex.color,
            vertex.texture_coords,
        )
    }
}
#[derive(Debug)]
pub struct TriangleRasterization {
    pub verteies: [VertexRasterization; 3],
}

impl Default for TriangleRasterization {
    fn default() -> Self {
        TriangleRasterization {
            verteies: [VertexRasterization::default(); 3],
        }
    }
}

impl Clone for TriangleRasterization {
    fn clone(&self) -> Self {
        TriangleRasterization {
            verteies: [self.verteies[0], self.verteies[1], self.verteies[2]],
        }
    }
}

impl Copy for TriangleRasterization {}

impl TriangleRasterization {
    fn new(v0: &VertexRasterization, v1: &VertexRasterization, v2: &VertexRasterization) -> Self {
        TriangleRasterization {
            verteies: [*v0, *v1, *v2],
        }
    }
    pub fn from_triangle(triangle: &Triangle) -> Self {
        TriangleRasterization::new(
            &VertexRasterization::from_vertex(
                &triangle.verteies[0],
                homogeneous(triangle.verteies[0].pos),
            ),
            &VertexRasterization::from_vertex(
                &triangle.verteies[1],
                homogeneous(triangle.verteies[1].pos),
            ),
            &VertexRasterization::from_vertex(
                &triangle.verteies[2],
                homogeneous(triangle.verteies[2].pos),
            ),
        )
    }
    pub(in crate::rasterization) unsafe fn rasterization(
        &self,
        scene: *mut Scene,
        model: &Model,
        start_row: i32,
        start_col: i32,
        block_row: i32,
        block_col: i32,
    ) {
        let mut box_left: i32 = 0x7FFFFFFF;
        let mut box_right: i32 = 0;
        let mut box_bottom: i32 = 0x7FFFFFFF;
        let mut box_top: i32 = 0;
        for i in 0..3 {
            box_left = min(self.verteies[i].transform_pos.x as i32, box_left);
            box_right = max(self.verteies[i].transform_pos.x as i32, box_right);
            box_bottom = min(self.verteies[i].transform_pos.y as i32, box_bottom);
            box_top = max(self.verteies[i].transform_pos.y as i32, box_top);
        }
        box_left = clamp(box_left, start_col, start_col + block_col - 1);
        box_right = clamp(box_right, start_col, start_col + block_col - 1);
        box_bottom = clamp(box_bottom, start_row, start_row + block_row - 1);
        box_top = clamp(box_top, start_row, start_row + block_row - 1);
        let edge1 = self.verteies[1].pos - self.verteies[0].pos;
        let edge2 = self.verteies[2].pos - self.verteies[1].pos;
        let uv_edge1 = self.verteies[1].texture_coords - self.verteies[0].texture_coords;
        let uv_edge2 = self.verteies[2].texture_coords - self.verteies[1].texture_coords;
        let tangent: Vector3f;
        let binormal: Vector3f;
        let diffuse_texture = model.get_texture(TextureTypes::Diffuse);
        let specular_texture = model.get_texture(TextureTypes::Specular);
        let normal_texture = model.get_texture(TextureTypes::Normal);
        let glow_texture = model.get_texture(TextureTypes::Glow);
        if let Some(_normal_texture) = &normal_texture {
            if uv_edge1.x * uv_edge2.y - uv_edge2.x * uv_edge1.y > 0.0 {
                tangent = (uv_edge2.y * edge1 - uv_edge1.y * edge2).normalize();
                binormal = -(uv_edge2.x * edge1 - uv_edge1.x * edge2).normalize();
            } else {
                tangent = -(uv_edge2.y * edge1 - uv_edge1.y * edge2).normalize();
                binormal = (uv_edge2.x * edge1 - uv_edge1.x * edge2).normalize();
            }
        } else {
            tangent = Vector3f::default();
            binormal = Vector3f::default();
        }
        for y in box_bottom..=box_top {
            for x in box_left..=box_right {
                let (mut alpha, mut beta, mut gamma) = cal_bary_coord_2d(
                    self.verteies[0].transform_pos.xy(),
                    self.verteies[1].transform_pos.xy(),
                    self.verteies[2].transform_pos.xy(),
                    Vector2f::new(x as f32 + 0.5, y as f32 + 0.5),
                );
                if is_inside_triangle(alpha, beta, gamma) {
                    alpha /= -self.verteies[0].transform_pos.w;
                    beta /= -self.verteies[1].transform_pos.w;
                    gamma /= -self.verteies[2].transform_pos.w;
                    let w_inter = 1.0 / (alpha + beta + gamma);
                    alpha *= w_inter;
                    beta *= w_inter;
                    gamma *= w_inter;
                    let point_transform_z = alpha * self.verteies[0].transform_pos.z
                        + beta * self.verteies[1].transform_pos.z
                        + gamma * self.verteies[2].transform_pos.z;
                    let point_transform_w = alpha * self.verteies[0].transform_pos.w
                        + beta * self.verteies[1].transform_pos.w
                        + gamma * self.verteies[2].transform_pos.w;
                    let point_transform_z = ((point_transform_z / -point_transform_w) + 1.0) * 0.5;
                    let idx = (*scene).get_index(x, y) as usize;
                    if (point_transform_z < (*scene).z_buffer[idx]) {
                        (*scene).z_buffer[idx] = point_transform_z;
                        let point_pos = alpha * self.verteies[0].pos
                            + beta * self.verteies[1].pos
                            + gamma * self.verteies[2].pos;
                        let point_normal = (alpha * self.verteies[0].normal
                            + beta * self.verteies[1].normal
                            + gamma * self.verteies[2].normal)
                            .normalize();
                        let point_uv = (alpha * self.verteies[0].texture_coords
                            + beta * self.verteies[1].texture_coords
                            + gamma * self.verteies[2].texture_coords);

                        (*scene).pos_buffer[idx] = point_pos;
                        if let Some(normal_texture) = &normal_texture {
                            let tbn_normal = (2.0 * normal_texture.get_rgb(point_uv.x, point_uv.y)
                                - Vector3f::new(1.0, 1.0, 1.0))
                            .normalize();
                            let tangent_orthogonal =
                                (tangent - tangent.dot(&point_normal) * point_normal).normalize();
                            let mut binormal_orthogonal =
                                point_normal.cross(&tangent_orthogonal).normalize();
                            if binormal_orthogonal.dot(&binormal) < 0.0 {
                                binormal_orthogonal = -binormal_orthogonal;
                            }
                            let tbn_matrix = Matrix3f::from_columns(&[
                                tangent_orthogonal,
                                binormal_orthogonal,
                                point_normal,
                            ]);
                            (*scene).normal_buffer[idx] = tbn_matrix * tbn_normal;
                        } else {
                            (*scene).normal_buffer[idx] = point_normal;
                        }
                        if let Some(diffuse_texture) = &diffuse_texture {
                            (*scene).diffuse_buffer[idx] =
                                diffuse_texture.get_rgb(point_uv.x, point_uv.y);
                        } else {
                            (*scene).diffuse_buffer[idx] = alpha * self.verteies[0].color
                                + beta * self.verteies[1].color
                                + gamma * self.verteies[2].color;
                        }
                        if let Some(specular_texture) = &specular_texture {
                            (*scene).specular_buffer[idx] =
                                specular_texture.get_rgb(point_uv.x, point_uv.y);
                        } else {
                            (*scene).specular_buffer[idx] = Vector3f::new(0.8, 0.8, 0.8);
                        }
                        if let Some(glow_texture) = &glow_texture {
                            (*scene).glow_buffer[idx] =
                                glow_texture.get_rgb(point_uv.x, point_uv.y);
                        } else {
                            (*scene).glow_buffer[idx] = Vector3f::default();
                        }
                    }
                }
            }
        }
    }
    pub(in crate::rasterization) unsafe fn rasterization_shadow_map<const IS_PROJECTION: bool>(
        &self,
        z_buffer: *mut Vec<f32>,
        start_row: i32,
        start_col: i32,
        block_row: i32,
        block_col: i32,
        depth_transformer: &impl Fn(f32, f32) -> f32,
        get_index: &impl Fn(i32, i32) -> i32,
    ) {
        let mut box_left: i32 = 0x7FFFFFFF;
        let mut box_right: i32 = 0;
        let mut box_bottom: i32 = 0x7FFFFFFF;
        let mut box_top: i32 = 0;
        for i in 0..3 {
            box_left = min(self.verteies[i].transform_pos.x as i32, box_left);
            box_right = max(self.verteies[i].transform_pos.x as i32, box_right);
            box_bottom = min(self.verteies[i].transform_pos.y as i32, box_bottom);
            box_top = max(self.verteies[i].transform_pos.y as i32, box_top);
        }
        box_left = clamp(box_left, start_col, start_col + block_col - 1);
        box_right = clamp(box_right, start_col, start_col + block_col - 1);
        box_bottom = clamp(box_bottom, start_row, start_row + block_row - 1);
        box_top = clamp(box_top, start_row, start_row + block_row - 1);
        for y in box_bottom..=box_top {
            for x in box_left..=box_right {
                let (mut alpha, mut beta, mut gamma) = cal_bary_coord_2d(
                    self.verteies[0].transform_pos.xy(),
                    self.verteies[1].transform_pos.xy(),
                    self.verteies[2].transform_pos.xy(),
                    Vector2f::new(x as f32 + 0.5, y as f32 + 0.5),
                );
                if is_inside_triangle(alpha, beta, gamma) {
                    if IS_PROJECTION {
                        alpha /= -self.verteies[0].transform_pos.w;
                        beta /= -self.verteies[1].transform_pos.w;
                        gamma /= -self.verteies[2].transform_pos.w;
                        let w_inter = 1.0 / (alpha + beta + gamma);
                        alpha *= w_inter;
                        beta *= w_inter;
                        gamma *= w_inter;
                    }
                    let point_transform_z = alpha * self.verteies[0].transform_pos.z
                        + beta * self.verteies[1].transform_pos.z
                        + gamma * self.verteies[2].transform_pos.z;
                    let point_transform_w = alpha * self.verteies[0].transform_pos.w
                        + beta * self.verteies[1].transform_pos.w
                        + gamma * self.verteies[2].transform_pos.w;
                    let depth = depth_transformer(point_transform_z, point_transform_w);
                    let idx = get_index(x, y) as usize;
                    unsafe {
                        if depth > (*z_buffer)[idx] {
                            (*z_buffer)[idx] = depth;
                        }
                    }
                }
            }
        }
    }
    fn clip_triangles<const N: usize, const IS_LESS: bool>(
        output: &mut Vec<TriangleRasterization>,
    ) {
        let old_size: usize = output.len();
        let mut remain_size: usize = 0;
        for i in 0..old_size {
            let mut vertex_out_cnt = 0;
            let mut vertex_unavailable = [false; 3];
            for j in 0..3 {
                if IS_LESS {
                    if output[i].verteies[j].transform_pos[N]
                        < output[i].verteies[j].transform_pos.w
                    {
                        vertex_out_cnt += 1;
                        vertex_unavailable[j] = true;
                    }
                } else {
                    if output[i].verteies[j].transform_pos[N]
                        > -output[i].verteies[j].transform_pos.w
                    {
                        vertex_out_cnt += 1;
                        vertex_unavailable[j] = true;
                    }
                }
            }
            match vertex_out_cnt {
                0 => {
                    if remain_size != i {
                        output[remain_size] = output[i];
                    }
                    remain_size += 1;
                }
                1 => {
                    let mut idx: [usize; 3] = [0; 3];
                    if vertex_unavailable[0] {
                        idx[0] = 1;
                        idx[1] = 2;
                        idx[2] = 0;
                    } else if vertex_unavailable[1] {
                        idx[0] = 2;
                        idx[1] = 0;
                        idx[2] = 1;
                    } else {
                        idx[0] = 0;
                        idx[1] = 1;
                        idx[2] = 2;
                    }
                    let a = &output[i].verteies[idx[0]];
                    let b = &output[i].verteies[idx[1]];
                    let c = &output[i].verteies[idx[2]];
                    let t_d: f32;
                    let t_e: f32;
                    if IS_LESS {
                        t_d = (a.transform_pos[N] - a.transform_pos.w)
                            / ((a.transform_pos[N] - a.transform_pos.w)
                                - (c.transform_pos[N] - c.transform_pos.w));
                        t_e = (b.transform_pos[N] - b.transform_pos.w)
                            / ((b.transform_pos[N] - b.transform_pos.w)
                                - (c.transform_pos[N] - c.transform_pos.w));
                    } else {
                        t_d = (a.transform_pos[N] + a.transform_pos.w)
                            / ((a.transform_pos[N] + a.transform_pos.w)
                                - (c.transform_pos[N] + c.transform_pos.w));
                        t_e = (b.transform_pos[N] + b.transform_pos.w)
                            / ((b.transform_pos[N] + b.transform_pos.w)
                                - (c.transform_pos[N] + c.transform_pos.w));
                    }
                    let d = (1.0 - t_d) * a + &(t_d * c);
                    let e = (1.0 - t_e) * b + &(t_e * c);
                    output.push(TriangleRasterization::new(b, &e, &d));
                    output[i].verteies[idx[2]] = d;
                    if remain_size != i {
                        output[remain_size] = output[i];
                    }
                    remain_size += 1;
                }
                2 => {
                    let mut idx: [usize; 3] = [0; 3];
                    if !vertex_unavailable[0] {
                        idx[0] = 0;
                        idx[1] = 1;
                        idx[2] = 2;
                    } else if !vertex_unavailable[1] {
                        idx[0] = 1;
                        idx[1] = 2;
                        idx[2] = 0;
                    } else {
                        idx[0] = 2;
                        idx[1] = 0;
                        idx[2] = 1;
                    }
                    let a = &output[i].verteies[idx[0]];
                    let b = &output[i].verteies[idx[1]];
                    let c = &output[i].verteies[idx[2]];
                    let t_d: f32;
                    let t_e: f32;
                    if IS_LESS {
                        t_d = (a.transform_pos[N] - a.transform_pos.w)
                            / ((a.transform_pos[N] - a.transform_pos.w)
                                - (b.transform_pos[N] - b.transform_pos.w));
                        t_e = (a.transform_pos[N] - a.transform_pos.w)
                            / ((a.transform_pos[N] - a.transform_pos.w)
                                - (c.transform_pos[N] - c.transform_pos.w));
                    } else {
                        t_d = (a.transform_pos[N] + a.transform_pos.w)
                            / ((a.transform_pos[N] + a.transform_pos.w)
                                - (b.transform_pos[N] + b.transform_pos.w));
                        t_e = (a.transform_pos[N] + a.transform_pos.w)
                            / ((a.transform_pos[N] + a.transform_pos.w)
                                - (c.transform_pos[N] + c.transform_pos.w));
                    }
                    let d = (1.0 - t_d) * a + &(t_d * b);
                    let e = (1.0 - t_e) * a + &(t_e * c);
                    output[i].verteies[idx[1]] = d;
                    output[i].verteies[idx[2]] = e;
                    if remain_size != i {
                        output[remain_size] = output[i];
                    }
                    remain_size += 1;
                }
                _ => {}
            }
        }
        output.copy_within(old_size.., remain_size);
        output.truncate(output.len() - old_size + remain_size);
    }
    pub(in crate::rasterization) fn to_NDC(&mut self, width: u32, height: u32) {
        for i in 0..3 {
            //z不需要齐次化
            self.verteies[i].transform_pos.x /= self.verteies[i].transform_pos.w;
            self.verteies[i].transform_pos.y /= self.verteies[i].transform_pos.w;
            self.verteies[i].transform_pos.x =
                (self.verteies[i].transform_pos.x + 1.0) * 0.5 * width as f32;
            self.verteies[i].transform_pos.y =
                (self.verteies[i].transform_pos.y + 1.0) * 0.5 * height as f32;
        }
    }
}
pub(in crate::rasterization) fn cal_bary_coord_2d(
    v0: Vector2f,
    v1: Vector2f,
    v2: Vector2f,
    p: Vector2f,
) -> (f32, f32, f32) {
    let ab = v1 - v0;
    let bc = v2 - v1;
    let pa = v0 - p;
    let pb = v1 - p;
    let sabc = ab.x * bc.y - bc.x * ab.y;
    let spbc = pb.x * bc.y - bc.x * pb.y;
    let spab = pa.x * ab.y - ab.x * pa.y;
    let alpha = spbc / sabc;
    let gamma = spab / sabc;
    return (alpha, 1.0 - alpha - gamma, gamma);
}

pub(in crate::rasterization) fn is_inside_triangle(alpha: f32, beta: f32, gamma: f32) -> bool {
    return alpha > -EPSILON && beta > -EPSILON && gamma > -EPSILON;
}
