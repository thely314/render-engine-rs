/*!
A binary program that displays rendering results, with a movable camera.
*/
#![allow(unused)]
#[warn(missing_docs)]
mod camera;
mod light;

use std::sync::{Arc, Mutex, mpsc};
use std::time::{SystemTime, UNIX_EPOCH};
use slint::{Image, Rgb8Pixel, SharedPixelBuffer};

// TODO: key
// https://docs.slint.dev/latest/docs/slint/reference/keyboard-input/focusscope/
// TODO: mouse
// https://github.com/slint-ui/slint/issues/2770
// https://docs.slint.dev/latest/docs/slint/reference/gestures/toucharea/
use camera::{CameraController, InputEvent};
use light::LightManager;

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

   // let (light_tx, light_rx) = mpsc::channel();
   // let light_tx = Arc::new(Mutex::new(light_tx));
   // let light_tx_clone = Arc::clone(&light_tx);

   let ui_weak = ui.as_weak();
   let double_buffer_clone = Arc::clone(&buffer);
   // 渲染线程
   std::thread::spawn(move || {
      let mut scene = Scene::new(width as i32, height as i32);
      let floor = Arc::new(Mutex::new(Model::from_file(
         "./models/floor.obj",
         Color4D::new(0.5, 0.5, 0.5, 1.0),
      )));

      let model = Arc::new(Mutex::new(Model::from_file(
         "./models/diablo3/diablo3_pose.obj",
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

      model.lock()
         .unwrap()
         .set_texture(Some(diffuse_texture), model::TextureTypes::Diffuse);
      model.lock()
         .unwrap()
         .set_texture(Some(specular_texture), model::TextureTypes::Specular);
      model.lock()
         .unwrap()
         .set_texture(Some(normal_texture), model::TextureTypes::Normal);
      model.lock()
         .unwrap()
         .set_texture(Some(glow_texture), model::TextureTypes::Glow);

      // model.lock().unwrap().set_pos(Vector3f::new(0.0, -2.45, 0.0));
      // 设定模型大小为原来的2.5倍
      model.lock().unwrap().set_scale(2.5);

      floor.lock().unwrap().set_pos(Vector3f::new(0.0, -2.45, 0.0));
      scene.add_model(floor.clone());
      scene.add_model(model.clone());
      // 设置远平面距离
      scene.set_z_far(-100.0);

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
         (model.lock().unwrap().get_pos() - Vector3f::new(10.0, 10.0, 10.0)).normalize()
      );
      scene.add_light(directional_light);

      // 初始化摄像机控制器
      let mut camera_controller = CameraController::new();
      let init_dir = scene.get_view_dir();
      camera_controller.set_dir([init_dir[0], init_dir[1], init_dir[2]]);

      loop {
         // TODO
         // 处理所有输入事件
         while let Ok(event) = event_rx.try_recv() {
            camera_controller.handle_event(&event);
         }

         let mut db = double_buffer_clone.lock().unwrap();

         // 获取后台缓冲区并渲染
         let back_buffer = db.back_buffer();

         camera_controller.update();
         let pos_camera = camera_controller.get_pos();
         let dir_camera = camera_controller.get_dir();
         scene.set_eye_pos(Vector3f::new(pos_camera[0], pos_camera[1], pos_camera[2]));
         scene.set_view_dir(Vector3f::new(dir_camera[0], dir_camera[1], dir_camera[2]));
         scene.start_render();
         db.buffer_convert_fill_back(scene.get_frame_buffer());

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
