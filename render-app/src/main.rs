/*!
A binary program that displays rendering results, with a movable camera.
*/
#[warn(missing_docs)]
mod camera;

use std::sync::{Arc, Mutex};
use std::time::{SystemTime, UNIX_EPOCH};
use slint::{Image, Rgb8Pixel, SharedPixelBuffer};

slint::include_modules!();

fn main() -> Result<(), slint::PlatformError> {
   let ui = MainWindow::new()?;

   // 创建像素缓冲区
   let (width, height) = (700, 600);
   let buffer = Arc::new(Mutex::new(
      DoubleBuffer::new(width, height)
   ));

   let ui_weak = ui.as_weak();
   let double_buffer_clone = Arc::clone(&buffer);

   // 渲染线程
   std::thread::spawn(move || {
      loop {
         let mut db = double_buffer_clone.lock().unwrap();

         // 获取后台缓冲区并渲染
         let back_buffer = db.back_buffer();
         render_frame(back_buffer, width, height);

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
fn render_frame(buffer: &mut [Rgb8Pixel], width: u32, height: u32) {
   for y in 0..height {
      for x in 0..width {
         // 计算像素颜色（示例：渐变效果）
         let r = (x as f32 / width as f32 * 255.0) as u8;
         let g = (y as f32 / height as f32 * 255.0) as u8;
         let b;
         if x > (SystemTime::now().duration_since(UNIX_EPOCH).unwrap().as_millis() / 10 % 700) as u32 {
            b = 128;
         } else {
            b = 0
         }

         // 写入像素缓冲区
         buffer[(y * width + x) as usize].r = r;
         buffer[(y * width + x) as usize].g = g;
         buffer[(y * width + x) as usize].b = b;
      }
   }
}
