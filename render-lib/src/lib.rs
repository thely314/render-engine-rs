#![warn(missing_docs)]
/*!
The root lib of render-lib.
*/
pub mod rasterization;

pub mod util;

#[cfg(test)]
mod test {
    use std::env::current_dir;
    use std::path::{self, Path};
    use std::sync::{Arc, Mutex};

    use assimp::Color4D;

    use crate::rasterization::light::{Light, LightTrait};
    use crate::rasterization::model::Model;
    use crate::rasterization::scene::Scene;
    use crate::rasterization::spot_light::SpotLight;
    use crate::rasterization::*;
    use crate::util::math::*;
    #[test]
    pub fn test_import() {
        let model_path = "./models/floor.obj"; // 请替换为你的模型文件路径
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
    }
    #[test]
    pub fn test_scene() {
        let mut scene = Scene::default();
        let model = Arc::new(Mutex::new(Model::from_file(
            "./models/tallbox.obj",
            Color4D::new(0.5, 0.5, 0.5, 1.0),
        )));
        let floor = Arc::new(Mutex::new(Model::from_file(
            "./models/floor.obj",
            Color4D::new(0.5, 0.5, 0.5, 1.0),
        )));
        model
            .lock()
            .unwrap()
            .set_pos(Vector3f::new(0.0, -2.45, 0.0));
        floor
            .lock()
            .unwrap()
            .set_pos(Vector3f::new(0.0, -2.45, 0.0));
        scene.add_model(model.clone());
        scene.add_model(floor.clone());
        let spot_light = Arc::new(Mutex::new(SpotLight::default()));
        spot_light
            .lock()
            .unwrap()
            .set_pos(Vector3f::new(10.0, 10.0, 10.0));
        spot_light
            .lock()
            .unwrap()
            .set_intensity(Vector3f::new(250.0, 250.0, 250.0));
        spot_light
            .lock()
            .unwrap()
            .set_light_dir(-Vector3f::new(10.0, 10.0, 10.0).normalize());
        scene.set_eye_pos(Vector3f::new(0.0, 0.0, 7.0));
        scene.add_light(spot_light);
        // scene.add_light(light.clone());
        scene.start_render();
        scene.save_to_file(Path::new("output.png"));
        // for i in 0..36 {
        //     scene.start_render();
        //     let _ = scene.save_to_file(Path::new(&format!("output{}.png", i + 1)));
        //     model.lock().unwrap().modeling(&get_modeling_matrix(
        //         Vector3f::new(0.0, 1.0, 0.0),
        //         10.0,
        //         Vector3f::new(0.0, 0.0, 0.0),
        //     ));
        // }
    }
}
