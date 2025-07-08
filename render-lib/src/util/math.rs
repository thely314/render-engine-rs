use std::f32::consts::PI;

pub use nalgebra::clamp;
/// 误差量
pub const EPSILON: f32 = 1e-4;

pub type Vector2f = nalgebra::Vector2<f32>;
pub type Vector3f = nalgebra::Vector3<f32>;
pub type Vector4f = nalgebra::Vector4<f32>;
pub type Matrix3f = nalgebra::Matrix3<f32>;
pub type Matrix4f = nalgebra::Matrix4<f32>;
///斐波那契圆盘分布数组
pub const FIBONACCI_SPIRAL_DIRECTION: [[f32; 2]; 64] = [
    [1.000000, 0.000000],
    [-0.737369, 0.675490],
    [0.087426, -0.996171],
    [0.608439, 0.793601],
    [-0.984713, -0.174182],
    [0.843755, -0.536728],
    [-0.259604, 0.965715],
    [-0.460907, -0.887448],
    [0.939321, 0.343039],
    [-0.924346, 0.381556],
    [0.423846, -0.905734],
    [0.299284, 0.954164],
    [-0.865211, -0.501408],
    [0.976676, -0.214719],
    [-0.575129, 0.818062],
    [-0.128511, -0.991708],
    [0.764649, 0.644447],
    [-0.999146, 0.041318],
    [0.708829, -0.705380],
    [-0.046191, 0.998933],
    [-0.640709, -0.767784],
    [0.991069, 0.133347],
    [-0.820858, 0.571132],
    [0.219481, -0.975617],
    [0.497181, 0.867647],
    [-0.952693, -0.303935],
    [0.907791, -0.419423],
    [-0.386061, 0.922473],
    [-0.338452, -0.940984],
    [0.885189, 0.465231],
    [-0.966970, 0.254890],
    [0.540838, -0.841127],
    [0.169376, 0.985551],
    [-0.790623, -0.612303],
    [0.996586, -0.082565],
    [-0.679079, 0.734065],
    [0.004878, -0.999988],
    [0.671885, 0.740655],
    [-0.995733, -0.092284],
    [0.796559, -0.604560],
    [-0.178984, 0.983852],
    [-0.532606, -0.846364],
    [0.964437, 0.264312],
    [-0.889686, 0.456572],
    [0.347617, -0.937637],
    [0.377043, 0.926196],
    [-0.903656, -0.428259],
    [0.955613, -0.294626],
    [-0.505622, 0.862755],
    [-0.209952, -0.977712],
    [0.815247, 0.579113],
    [-0.992323, 0.123671],
    [0.648169, -0.761496],
    [0.036443, 0.999336],
    [-0.701914, -0.712262],
    [0.998695, 0.051064],
    [-0.770900, 0.636956],
    [0.138180, -0.990407],
    [0.567121, 0.823635],
    [-0.974534, -0.224238],
    [0.870062, -0.492942],
    [-0.308579, 0.951199],
    [-0.414989, -0.909826],
    [0.920579, 0.390557],
];

///将Vector3f提升到Vector4f，w分量填1.0
pub fn homogeneous(vec: Vector3f) -> Vector4f {
    //用它的原因是nalgebra的Vector类型的to_homogeneous()的w分量填的0.0，Point类型填的1.0
    //但写了一堆代码之后只改一个函数调用显然比底层类型全换一遍方便
    //而且不知道为什么Point没有dot和cross接口
    Vector4f::new(vec.x, vec.y, vec.z, 1.0)
}
///生成modeling矩阵，以axis为竖直向上的旋转轴，逆时针旋转angle度（角度制），之后平移movement距离
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
/// 根据摄像机位置和视线方向生成view矩阵，向上的向量取(0,1,0)，根据摄像机位置做正交处理
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
/// 生成透视投影矩阵，fov为竖直方向的fov，z_near和z_far需传入负值
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

/// 给penumbra_mask做水平方向盒式滤波
pub(crate) fn blur_penumbra_mask_horizontal(
    input: &Vec<f32>,
    width: i32,
    height: i32,
    radius: i32,
    get_index: &impl Fn(i32, i32) -> i32,
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

/// 给penumbra_mask做竖直方向盒式滤波
pub(crate) fn blur_penumbra_mask_vertical(
    input: &Vec<f32>,
    width: i32,
    height: i32,
    radius: i32,
    get_index: &impl Fn(i32, i32) -> i32,
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

/// 根据给定sample_index生成长度为sample_index*sample_count_inverse，方向为斐波那契圆盘分布数组的第sample_index项的二维向量
/// clump_exponent用来控制偏好采样中心的点还是边缘的点
pub fn compute_fibonacci_spiral_disk_sample_uniform(
    sample_index: i32,
    sample_count_inverse: f32,
    clump_exponent: f32,
    sample_dist_norm: f32,
) -> Vector2f {
    let sample_dist_norm = (sample_index as f32 * sample_count_inverse).powf(0.5 * clump_exponent);
    sample_dist_norm
        * Vector2f::new(
            FIBONACCI_SPIRAL_DIRECTION[sample_index as usize][0],
            FIBONACCI_SPIRAL_DIRECTION[sample_index as usize][1],
        )
}
