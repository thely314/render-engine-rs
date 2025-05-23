use std::{clone, default};

pub type Vector2f = nalgebra::Vector2<f32>;
pub type Vector3f = nalgebra::Vector3<f32>;
pub type Vector4f = nalgebra::Vector4<f32>;
pub type Matrix3f = nalgebra::Matrix3<f32>;
pub type Matrix4f = nalgebra::Matrix4<f32>;

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
    fn new(v0: &Vertex, v1: &Vertex, v2: &Vertex) -> Self {
        Triangle {
            verteies: [*v0, *v1, *v2],
        }
    }
    fn clip(mvp: &Matrix4f, mv: &Matrix4f, output: &mut Vec<TriangleRasterization>) {}
    fn modeling(&mut self, matrix: &Matrix4f) {
        for i in 0..3 {
            self.verteies[i].pos = (matrix * self.verteies[i].pos.to_homogeneous())
                .fixed_rows::<3>(0)
                .into();
            self.verteies[i].normal = matrix.fixed_view::<3, 3>(0, 0) * self.verteies[i].normal;
        }
    }
}

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
                triangle.verteies[0].pos.to_homogeneous(),
            ),
            &VertexRasterization::from_vertex(
                &triangle.verteies[1],
                triangle.verteies[1].pos.to_homogeneous(),
            ),
            &VertexRasterization::from_vertex(
                &triangle.verteies[2],
                triangle.verteies[2].pos.to_homogeneous(),
            ),
        )
    }
    pub fn clip_triangles<const N: usize, const IS_LESS: bool>(
        output: &mut Vec<TriangleRasterization>,
    ) {
        let old_size: usize = output.len();
        let mut remain_size: usize = 0;
        for i in 0..old_size {
            let mut vertex_out_cnt = 0;
            let mut vertex_unavailable = [false; 3];
            for j in 0..3 {
                if IS_LESS {
                    if (output[i].verteies[j].transform_pos[N]
                        < -output[i].verteies[j].transform_pos.w)
                    {
                        vertex_out_cnt += 1;
                        vertex_unavailable[j] = true;
                    }
                } else {
                    if (output[i].verteies[j].transform_pos[N]
                        > output[i].verteies[j].transform_pos.w)
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
                        idx[1] = 1;
                        idx[2] = 0;
                    } else {
                        idx[0] = 0;
                        idx[1] = 1;
                        idx[2] = 2;
                    }
                    let A = &output[i].verteies[idx[0]];
                    let B = &output[i].verteies[idx[1]];
                    let C = &output[i].verteies[idx[2]];
                    let mut t_D: f32 = 0.0;
                    let mut t_E: f32 = 0.0;
                    if IS_LESS {
                        t_D = (A.transform_pos[N] + A.transform_pos.w)
                            / ((A.transform_pos[N] + A.transform_pos.w)
                                - (C.transform_pos[N] + C.transform_pos.w));
                        t_E = (B.transform_pos[N] + B.transform_pos.w)
                            / ((B.transform_pos[N] + B.transform_pos.w)
                                - (C.transform_pos[N] + C.transform_pos.w));
                    } else {
                        t_D = (A.transform_pos[N] - A.transform_pos.w)
                            / ((A.transform_pos[N] - A.transform_pos.w)
                                - (C.transform_pos[N] - C.transform_pos.w));
                        t_E = (B.transform_pos[N] - B.transform_pos.w)
                            / ((B.transform_pos[N] - B.transform_pos.w)
                                - (C.transform_pos[N] - C.transform_pos.w));
                    }
                    let D = (1.0 - t_D) * A + &(t_D * C);
                    let E = (1.0 - t_E) * B + &(t_E * C);
                    output.push(TriangleRasterization::new(A, &D, &E));
                    output[i].verteies[idx[2]] = D;
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
                        idx[1] = 1;
                        idx[2] = 0;
                    }
                    let A = &output[i].verteies[idx[0]];
                    let B = &output[i].verteies[idx[1]];
                    let C = &output[i].verteies[idx[2]];
                    let mut t_D: f32 = 0.0;
                    let mut t_E: f32 = 0.0;
                    if IS_LESS {
                        t_D = (A.transform_pos[N] + A.transform_pos.w)
                            / ((A.transform_pos[N] + A.transform_pos.w)
                                - (B.transform_pos[N] + B.transform_pos.w));
                        t_E = (A.transform_pos[N] + A.transform_pos.w)
                            / ((A.transform_pos[N] + A.transform_pos.w)
                                - (C.transform_pos[N] + C.transform_pos.w));
                    } else {
                        t_D = (A.transform_pos[N] - A.transform_pos.w)
                            / ((A.transform_pos[N] - A.transform_pos.w)
                                - (B.transform_pos[N] - B.transform_pos.w));
                        t_E = (A.transform_pos[N] - A.transform_pos.w)
                            / ((A.transform_pos[N] - A.transform_pos.w)
                                - (C.transform_pos[N] - C.transform_pos.w));
                    }
                    let D = (1.0 - t_D) * A + &(t_D * B);
                    let E = (1.0 - t_E) * A + &(t_E * C);
                    output[i].verteies[idx[1]] = D;
                    output[i].verteies[idx[2]] = E;
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
}
