#![warn(missing_docs)]
/*!
The root lib of render-lib.
*/
pub mod rasterization;

pub mod util;

#[cfg(test)]
mod test {
    use crate::rasterization::{
        triangle::Vector2f,
        triangle::{Vector3f, Vertex},
    };

    use super::*;
    #[test]
    pub fn test_vertex() {
        let model_path = "./models/utah_teapot.obj"; // 请替换为你的模型文件路径
                                                     // 创建一个新的Importer实例
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
        match importer.read_file(model_path) {
            Ok(scene) => {
                println!("成功加载模型: {}", model_path);
                let mut total_faces = 0;
                // 遍历场景中的所有网格 (meshes)
                for mesh in scene.mesh_iter() {
                    total_faces += mesh.num_faces();
                }
                println!("模型中的总面数: {}", total_faces);
            }
            Err(err) => {
                eprintln!("加载模型失败: {}", err);
            }
        }
        let v = Vertex::new(
            Vector3f::new(0.0, 0.0, 0.0),
            Vector3f::new(1.0, 1.0, 1.0),
            Vector3f::new(1.0, 1.0, 1.0),
            Vector2f::new(1.0, 1.0),
        );
        let u = (0.5 as f32) * &v;
        println!("{:?}", v);
        println!("{:?}", u);
        println!("{:?}", u + &v);
    }
}
