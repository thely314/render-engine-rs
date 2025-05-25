use std::f32::consts::PI;

use nalgebra::{clamp, Matrix};

pub const EPSILON: f32 = 1e-4;
pub type Vector2f = nalgebra::Vector2<f32>;
pub type Vector3f = nalgebra::Vector3<f32>;
pub type Vector4f = nalgebra::Vector4<f32>;
pub type Matrix3f = nalgebra::Matrix3<f32>;
pub type Matrix4f = nalgebra::Matrix4<f32>;

pub fn get_modeling_matrix(axis: Vector3f, angle: f32, movement: Vector3f) -> Matrix4f {
    let axis = axis.normalize();
    let angle = angle * PI / 180.0;
    let cos_val = angle.cos();
    let sin_val = angle.sin();
    let axis_cross = Matrix3f::from_row_slice(&[
        0.0, -axis.z, axis.y, axis.z, 0.0, -axis.x, -axis.y, axis.x, 0.0,
    ]);
    let rotate_matrix = sin_val * axis_cross
        + cos_val * Matrix3f::identity()
        + (1.0 - cos_val) * axis * &axis.transpose();
    Matrix4f::from_row_slice(&[
        rotate_matrix[(0, 0)],
        rotate_matrix[(0, 1)],
        rotate_matrix[(0, 2)],
        movement.x,
        rotate_matrix[(1, 0)],
        rotate_matrix[(1, 1)],
        rotate_matrix[(1, 2)],
        movement.y,
        rotate_matrix[(2, 0)],
        rotate_matrix[(2, 1)],
        rotate_matrix[(2, 2)],
        movement.z,
        0.0,
        0.0,
        0.0,
        1.0,
    ])
}
pub fn get_view_matrix(eye_pos: Vector3f, view_dir: Vector3f) -> Matrix4f {
    let view_dir: Vector3f = view_dir.normalize();
    let sky_dir = (Vector3f::y() - Vector3f::y().dot(&view_dir) * view_dir).normalize();
    let x_dir = view_dir.cross(&sky_dir).normalize();
    let sub_xyz_matrix = Matrix3f::from_rows(&[
        x_dir.transpose(),
        sky_dir.transpose(),
        -view_dir.transpose(),
    ]);
    let mut move_matrix = Matrix4f::identity();
    move_matrix[(0, 3)] = -eye_pos.x;
    move_matrix[(1, 3)] = -eye_pos.y;
    move_matrix[(2, 3)] = -eye_pos.z;
    let mut xyz_matrix = Matrix4f::identity();
    xyz_matrix
        .fixed_view_mut::<3, 3>(0, 0)
        .copy_from(&sub_xyz_matrix);
    xyz_matrix * move_matrix
}
pub fn get_projection_matrix(fov: f32, aspect_ratio: f32, z_near: f32, z_far: f32) -> Matrix4f {
    let tan_val_div = 1.0 / (fov / 360.0 * PI).tan();
    Matrix4f::from_row_slice(&[
        -tan_val_div / aspect_ratio,
        0.0,
        0.0,
        0.0,
        0.0,
        -tan_val_div,
        0.0,
        0.0,
        0.0,
        0.0,
        (z_near + z_far) / (z_near - z_far),
        -2.0 * z_near * z_far / (z_near - z_far),
        0.0,
        0.0,
        1.0,
        0.0,
    ])
}
pub(crate) fn blur_penumbra_mask_horizontal<T: Fn(i32, i32) -> i32>(
    input: &Vec<f32>,
    width: i32,
    height: i32,
    radius: i32,
    get_index: T,
) -> Vec<f32> {
    let mut output: Vec<f32> = Vec::with_capacity(input.len());
    output.resize(input.len(), 0.0);
    for y in 0..height {
        for x in 0..width {
            let mut sum: f32 = 0.0;
            for offset in -radius..=radius {
                sum += input[get_index(clamp(x + offset, 0, width - 1), y) as usize];
            }
            output[get_index(x, y) as usize] = sum / (2 * radius + 1) as f32;
        }
    }
    output
}
pub(crate) fn blur_penumbra_mask_vertical<T: Fn(i32, i32) -> i32>(
    input: &Vec<f32>,
    width: i32,
    height: i32,
    radius: i32,
    get_index: T,
) -> Vec<f32> {
    let mut output: Vec<f32> = Vec::with_capacity(input.len());
    output.resize(input.len(), 0.0);
    for y in 0..height {
        for x in 0..width {
            let mut sum: f32 = 0.0;
            for offset in -radius..=radius {
                sum += input[get_index(x, clamp(y + offset, 0, height - 1)) as usize];
            }
            output[get_index(x, y) as usize] = sum / (2 * radius + 1) as f32;
        }
    }
    output
}
