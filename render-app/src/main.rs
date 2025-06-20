/*!
A binary program that displays rendering results, with a movable camera.
*/
#![allow(unused)]
#[warn(missing_docs)]
mod camera;

use std::sync::{Arc, Mutex, mpsc};
use std::time::{SystemTime, UNIX_EPOCH};
use slint::{Image, Rgb8Pixel, SharedPixelBuffer};

// TODO: key
// https://docs.slint.dev/latest/docs/slint/reference/keyboard-input/focusscope/
// TODO: mouse
// https://github.com/slint-ui/slint/issues/2770
// https://docs.slint.dev/latest/docs/slint/reference/gestures/toucharea/
use camera::{CameraController, InputEvent};

slint::include_modules!();

fn main() -> Result<(), slint::PlatformError> {
   let ui = MainWindow::new().unwrap();

   // 创建像素缓冲区
   let (width, height) = (700, 600);
   let buffer = Arc::new(Mutex::new(
      DoubleBuffer::new(width, height)
   ));

   let window_phy_pos = ui.window().position();
   let window_scale_factor = ui.window().scale_factor();
   println!("Window position: {:?}", window_phy_pos);
   println!("Window scale factor: {}", window_scale_factor);

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
   ui.on_send_key_released(move |event| {
         if let Some(tx) = event_tx_clone.lock().ok() {
         tx.send(InputEvent::KeyRelease(event.text.to_string())).unwrap();
      }
   });

   let ui_weak = ui.as_weak();
   let double_buffer_clone = Arc::clone(&buffer);
   // 渲染线程
   std::thread::spawn(move || {
      // 初始化摄像机控制器
      let mut camera_controller = CameraController::new();
      let mut last_mouse_position: Option<slint::PhysicalPosition> = None;

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
         render_frame(back_buffer, width, height, pos_camera[0], pos_camera[1]);

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
}

/// 示例渲染函数
fn render_frame(buffer: &mut [Rgb8Pixel], width: u32, height: u32, pos_x: u32, pos_y: u32) {
   for y in 0..height {
      for x in 0..width {
         // 计算像素颜色（示例：渐变效果）
         let mut r = (x as f32 / width as f32 * 255.0) as u8;
         let mut g = (y as f32 / height as f32 * 255.0) as u8;
         let mut b;
         if x > (SystemTime::now().duration_since(UNIX_EPOCH).unwrap().as_millis() / 10 % 700) as u32 {
            b = 128;
         } else {
            b = 0
         }
         if x.abs_diff(pos_x) < 100 && y.abs_diff(pos_y) < 100 {
            r = 0;
            g = 0;
            b = 0;
         }

         // 写入像素缓冲区
         buffer[(y * width + x) as usize].r = r;
         buffer[(y * width + x) as usize].g = g;
         buffer[(y * width + x) as usize].b = b;
      }
   }
}
