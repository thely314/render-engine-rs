/*!
A binary program that displays rendering results, with a movable camera.
*/
#![allow(unused)]
#[warn(missing_docs)]
mod camera;
mod light;

use std::sync::{Arc, Mutex, mpsc};
use std::time::{SystemTime, UNIX_EPOCH};
use slint::{Image, Rgb8Pixel, SharedPixelBuffer, ComponentHandle, ModelRc, VecModel};
use rfd::FileDialog;
use std::path::{Path, PathBuf};

use camera::{CameraController, InputEvent};
use light::{LightIdentifier, LightManager};

use render_lib::assimp::Color4D;
use render_lib::nalgebra::clamp;
use render_lib::rasterization::{
   // light::Light,
   light::LightTrait,
   model::Model,
   scene::Scene,
   spot_light::SpotLight,
   directional_light::DirectionalLight,
   texture::Texture
};
use render_lib::{
   rasterization::*,
   util::math::*
};

slint::include_modules!();

const WIDTH: u32 = 700;
const HEIGHT: u32 = 600;

fn main() -> Result<(), slint::PlatformError> {
   let ui = MainWindow::new().unwrap();

   // 创建像素缓冲区
   let (width, height) = (WIDTH, HEIGHT);
   let buffer = Arc::new(Mutex::new(
      DoubleBuffer::new(width, height)
   ));

   let (event_tx, event_rx) = mpsc::channel();
   let event_tx = Arc::new(Mutex::new(event_tx));

   // 设置Slint回调
   let event_tx_clone = Arc::clone(&event_tx);
   ui.on_send_key_pressed(move |event| {
      if let Some(tx) = event_tx_clone.lock().ok() {
         tx.send(InputEvent::KeyPress(event.text.to_string())).unwrap();
      }
   });

   let event_tx_clone = Arc::clone(&event_tx);
   ui.on_send_moved(move |x, y| {
         if let Some(tx) = event_tx_clone.lock().ok() {
         tx.send(InputEvent::MouseMove(x, y)).unwrap();
      }
   });

   let event_tx_clone = Arc::clone(&event_tx);
   ui.on_send_mouse_click(move || {
         if let Some(tx) = event_tx_clone.lock().ok() {
         tx.send(InputEvent::MouseClick).unwrap();
      }
   });

   let event_tx_clone = Arc::clone(&event_tx);
   ui.on_send_key_released(move |event| {
         if let Some(tx) = event_tx_clone.lock().ok() {
         tx.send(InputEvent::KeyRelease(event.text.to_string())).unwrap();
      }
   });

   let ui_weak = ui.as_weak();
   let double_buffer_clone = Arc::clone(&buffer);

   let mut scene = Scene::new(width as i32, height as i32);
   // 用于渲染线程
   let mut scene_arc = Arc::new(Mutex::new(scene));
   // 用于 light 管理线程
   let scene_clone_light = Arc::clone(&scene_arc);
   // 用于 model 加载线程
   let scene_clone_model = Arc::clone(&scene_arc);

   // 当前 model 路径主线程记录
   let mut current_model_path = String::from("./models/diablo3/diablo3_pose.obj");

   // model 回调实现持有
   let mut old_model = Arc::new(Mutex::new(Model::from_file(
      &current_model_path,
      Color4D::new(0.5, 0.5, 0.5, 1.0),
   )));
   // 渲染线程持有
   let model_clone = Arc::clone(&old_model);

   // 渲染线程
   std::thread::spawn(move || {
      // let mut scene = Scene::new(width as i32, height as i32);
      let floor = Arc::new(Mutex::new(Model::from_file(
         "./models/floor.obj",
         Color4D::new(0.5, 0.5, 0.5, 1.0),
      )));

      //desired_channels指定读入纹理的期望通道数，这里期望为rgb三通道图
      // Diffuse漫反射贴图
      let diffuse_texture = Arc::new(Texture::new(
         "./models/diablo3/diablo3_pose_diffuse.tga",
         Some(3),
      ));
      // Specular高光贴图
      let specular_texture = Arc::new(Texture::new(
         "./models/diablo3/diablo3_pose_spec.tga",
         Some(3),
      ));
      // Normal法线贴图
      let normal_texture = Arc::new(Texture::new(
         "./models/diablo3/diablo3_pose_nm_tangent.tga",
         Some(3),
      ));
      // Glow自发光贴图
      let glow_texture = Arc::new(Texture::new(
         "./models/diablo3/diablo3_pose_glow.tga",
         Some(3),
      ));

      model_clone.lock()
         .unwrap()
         .set_texture(Some(diffuse_texture), model::TextureTypes::Diffuse);
      model_clone.lock()
         .unwrap()
         .set_texture(Some(specular_texture), model::TextureTypes::Specular);
      model_clone.lock()
         .unwrap()
         .set_texture(Some(normal_texture), model::TextureTypes::Normal);
      model_clone.lock()
         .unwrap()
         .set_texture(Some(glow_texture), model::TextureTypes::Glow);

      // 设定模型大小为原来的2.5倍
      model_clone.lock().unwrap().set_scale(2.5);

      floor.lock().unwrap().set_pos(Vector3f::new(0.0, -2.45, 0.0));
      scene_arc.lock().unwrap().add_model(floor.clone());
      scene_arc.lock().unwrap().add_model(model_clone.clone());
      // 设置远平面距离
      scene_arc.lock().unwrap().set_z_far(-100.0);

      // 初始环境光源
      // 光源 spot
      // let spot_light = Arc::new(Mutex::new(SpotLight::default()));
      // spot_light.lock().unwrap().set_pos(Vector3f::new(10.0, 10.0, 10.0));
      // spot_light.lock().unwrap().set_intensity(Vector3f::new(250.0, 250.0, 250.0));
      // spot_light.lock().unwrap().set_light_dir(-Vector3f::new(10.0, 10.0, 10.0).normalize());
      // scene.add_light(spot_light);

      // 光源 directional
      let directional_light = Arc::new(Mutex::new(DirectionalLight::default()));
      // directional_light.lock().unwrap().set_shadow_status(false);
      directional_light.lock().unwrap().set_pos(Vector3f::new(10.0, 10.0, 10.0));
      directional_light.lock().unwrap().set_intensity(Vector3f::new(1.0, 1.0, 1.0));
      directional_light.lock().unwrap().set_light_dir(
         (model_clone.lock().unwrap().get_pos() - Vector3f::new(10.0, 10.0, 10.0)).normalize()
      );
      scene_arc.lock().unwrap().add_light(directional_light);

      // 初始化摄像机控制器
      let mut camera_controller = CameraController::new();
      let init_dir = scene_arc.lock().unwrap().get_view_dir();
      camera_controller.set_dir([init_dir[0], init_dir[1], init_dir[2]]);

      loop {
         while let Ok(event) = event_rx.try_recv() {
            camera_controller.handle_event(&event);
         }

         let mut db = double_buffer_clone.lock().unwrap();

         // 获取后台缓冲区并渲染
         let back_buffer = db.back_buffer();

         camera_controller.update();
         let pos_camera = camera_controller.get_pos();
         let dir_camera = camera_controller.get_dir();
         scene_arc.lock().unwrap().set_eye_pos(Vector3f::new(pos_camera[0], pos_camera[1], pos_camera[2]));
         scene_arc.lock().unwrap().set_view_dir(Vector3f::new(dir_camera[0], dir_camera[1], dir_camera[2]));
         scene_arc.lock().unwrap().start_render();
         db.buffer_convert_fill_back(scene_arc.lock().unwrap().get_frame_buffer());

         // 交换缓冲区
         db.swap_buffers();

         // 更新UI
         let current_buffer = db.current_buffer().clone();
         let weak = ui_weak.clone();
         slint::invoke_from_event_loop(move || {
            weak.upgrade_in_event_loop(|ui| {
               ui.set_render_target(Image::from_rgb8(current_buffer));
            }).unwrap();
         }).unwrap();

         std::thread::sleep(std::time::Duration::from_millis(16));
      }
   });

   // light 回调通信
   let (light_tx, light_rx) = mpsc::channel();
   let light_tx = Arc::new(Mutex::new(light_tx));
   let light_tx_clone = Arc::clone(&light_tx);
   let mut light_manager = LightManager::new();

   // 获取全局的 LightManage 对象
   let light_manage = ui.global::<LightManage>();

   let light_tx_clone = Arc::clone(&light_tx);
   light_manage.on_add_light(move |identifier| {
         if let Some(tx) = light_tx_clone.lock().ok() {
         tx.send((identifier, true)).unwrap();
      }
   });

   let light_tx_clone = Arc::clone(&light_tx);
   light_manage.on_del_light(move |identifier| {
         if let Some(tx) = light_tx_clone.lock().ok() {
         tx.send((identifier, false)).unwrap();
      }
   });

   // light 管理线程
   std::thread::spawn(move || {
      loop {
         while let Ok((identifier, method)) = light_rx.try_recv() {
            match method {
               true => {
                  // add lights
                  match identifier.light_type.as_str() {
                     "Spot" => {
                        let light = light_manager.add_spot_light(identifier.into());
                        scene_clone_light.lock().unwrap().add_light(light);
                     },
                     "Directional" => {
                        let light = light_manager.add_directional_light(identifier.into());
                        scene_clone_light.lock().unwrap().add_light(light);
                     },
                     _ => {},
                  }
               },
               false => {
                  // delete lights
                  if let Some(light_arc) = light_manager.delete_light(identifier.into()) {
                     scene_clone_light.lock().unwrap().delete_light(&light_arc);
                  }
               },
            }
            // 更新slint light列表
            // let model = VecModel::default();
            // for light in light_manager.lights.values() {
            //    // let light = *light.lock().unwrap();
            //    // model.push(Light {});
            //    // hashmap 改成存储(Identifier, Arc<Mutex<dyn LightTrait>>)
            // }
            // 无法解决
            // 孤儿规则，没法为 ModelRC 实现sync，不能传递
            // light_manage.set_lights(ModelRc::new(model));
            // light_manage.set_lights(model);
         }
      }
   });

   // model 回调通信
   ui.on_load_model_main_req(move || {
      // 使用 rfd 库打开文件选择对话框
      if let Some(folder_path) = FileDialog::new()
         .set_directory(PathBuf::from("./models/diablo3"))
         .set_title("Select Model Folder")
         .pick_folder()
      {
         // 获取文件夹路径
         let folder_path_str = folder_path.to_string_lossy().to_string();

         // 在文件夹中查找纹理文件
         let mut diffuse_path = String::new();
         let mut specular_path = String::new();
         let mut normal_path = String::new();
         let mut glow_path = String::new();
        
         // 遍历文件夹中所有内容
         // 使用队列进行广度优先搜索
         let mut dirs_to_scan = vec![folder_path_str.clone()];
         while let Some(current_dir) = dirs_to_scan.pop() {
            if let Ok(entries) = std::fs::read_dir(&current_dir) {
                  for entry in entries {
                     if let Ok(entry) = entry {
                        let path = entry.path();
                        
                        // 如果是文件夹，加入待扫描队列
                        if path.is_dir() {
                            dirs_to_scan.push(path.to_string_lossy().to_string());
                            continue;
                        }
                        if let Some(file_name) = path.file_name().and_then(|s| s.to_str()) {
                           // 检查文件名是否包含特定模式
                           let file_lower = file_name.to_lowercase();
                           if file_lower.contains("pose") && file_lower.ends_with(".obj") {
                              // 更新 model 路径主线程记录
                              current_model_path = path.to_string_lossy().to_string();
                           } else if file_lower.contains("diffuse") && file_lower.ends_with(".tga") {
                              diffuse_path = path.to_string_lossy().to_string();
                           } else if file_lower.contains("spec") && file_lower.ends_with(".tga") {
                              specular_path = path.to_string_lossy().to_string();
                           } else if (file_lower.contains("normal") || file_lower.contains("tangent")) 
                                    && file_lower.ends_with(".tga") {
                              normal_path = path.to_string_lossy().to_string();
                           } else if file_lower.contains("glow") && file_lower.ends_with(".tga") {
                              glow_path = path.to_string_lossy().to_string();
                           }
                        }
                     }
                  }
            }
         }
         // 创建新模型
         let new_model = Arc::new(Mutex::new(Model::from_file(
            &current_model_path,
            Color4D::new(0.5, 0.5, 0.5, 1.0),
         )));
         let model_clone = Arc::clone(&new_model);
         // 设定模型大小为原来的2.5倍
         model_clone.lock().unwrap().set_scale(2.5);
         // 渲染接口:
         // 设置texture
         if !diffuse_path.is_empty() {
               let texture = Arc::new(Texture::new(&diffuse_path, Some(3)));
               model_clone.lock()
                  .unwrap()
                  .set_texture(Some(texture), model::TextureTypes::Diffuse);
         }
         
         if !specular_path.is_empty() {
               let texture = Arc::new(Texture::new(&specular_path, Some(3)));
               model_clone.lock()
                  .unwrap()
                  .set_texture(Some(texture), model::TextureTypes::Specular);
         }
         
         if !normal_path.is_empty() {
               let texture = Arc::new(Texture::new(&normal_path, Some(3)));
               model_clone.lock()
                  .unwrap()
                  .set_texture(Some(texture), model::TextureTypes::Normal);
         }
         
         if !glow_path.is_empty() {
               let texture = Arc::new(Texture::new(&glow_path, Some(3)));
               model_clone.lock()
                  .unwrap()
                  .set_texture(Some(texture), model::TextureTypes::Glow);
         }
         // 删除旧模型
         scene_clone_model.lock().unwrap().delete_model(&old_model);
         // 更新 old_model
         *old_model.lock().unwrap() = model_clone.lock().unwrap().clone();
         // 加载新模型
         scene_clone_model.lock().unwrap().add_model(model_clone.clone());
      }
   });

   ui.run()
}

/// 双缓冲实现
struct DoubleBuffer {
   buffers: [SharedPixelBuffer<Rgb8Pixel>; 2],
   current: usize,
}

impl DoubleBuffer {
   fn new(width: u32, height: u32) -> DoubleBuffer {
      let buffer0 = SharedPixelBuffer::new(width, height);
      let buffer1 = SharedPixelBuffer::new(width, height);
      Self {
         buffers: [buffer0, buffer1],
         current: 0,
      }
   }

   fn swap_buffers(&mut self) {
      self.current = 1 - self.current;
   }

   fn back_buffer(&mut self) -> &mut [Rgb8Pixel] {
      let index = 1 - self.current;
      self.buffers[index].make_mut_slice()
   }

   fn current_buffer(&self) -> &SharedPixelBuffer<Rgb8Pixel> {
      &self.buffers[self.current]
   }

   fn buffer_convert_fill_back(&mut self, src_buffer: &Vec<Vector3f>) -> () {
      let mut back_buffer = self.back_buffer();
      for y in 0..HEIGHT {
         for x in 0..WIDTH {
            let idx = (WIDTH * (HEIGHT - y - 1) + x) as usize;
            back_buffer[(WIDTH * y + x) as usize].r = ((clamp(src_buffer[idx][0], 0.0, 1.0) * 255.0).round() as u8);
            back_buffer[(WIDTH * y + x) as usize].g = ((clamp(src_buffer[idx][1], 0.0, 1.0) * 255.0).round() as u8);
            back_buffer[(WIDTH * y + x) as usize].b = ((clamp(src_buffer[idx][2], 0.0, 1.0) * 255.0).round() as u8);
         }
      }
   }
}

impl From<Light> for LightIdentifier {
   fn from(value: Light) -> Self {
       LightIdentifier {
           name: value.name,
           light_type: value.light_type,
           position_x: value.position_x,
           position_y: value.position_y,
           position_z: value.position_z,
           color_r: value.color_R,
           color_g: value.color_G,
           color_b: value.color_B,
           forward_x: value.forward_x,
           forward_y: value.forward_y,
           forward_z: value.forward_z,
       }
   }
}