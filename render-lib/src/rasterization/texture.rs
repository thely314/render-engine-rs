use image::{DynamicImage, GenericImageView};
use nalgebra::clamp;

use crate::util::math::*;

pub struct Texture {
    data: Vec<u8>,
    width: u32,
    height: u32,
    channels: u32,
}
impl Default for Texture {
    fn default() -> Self {
        Texture {
            data: Vec::default(),
            width: 0,
            height: 0,
            channels: 0,
        }
    }
}
impl Texture {
    pub fn new(filename: &std::path::Path, desired_channels: Option<u32>) -> Self {
        let img = image::ImageReader::open(filename)
            .expect("Failed to open image")
            .decode()
            .expect("Failed to decode image");
        let (width, height) = img.dimensions();
        match desired_channels {
            Some(1) => Texture {
                data: img.into_luma8().into_raw(),
                width,
                height,
                channels: 1,
            },
            Some(2) => Texture {
                data: img.into_luma_alpha8().into_raw(),
                width,
                height,
                channels: 2,
            },
            Some(3) => Texture {
                data: img.into_rgb8().into_raw(),
                width,
                height,
                channels: 3,
            },
            Some(4) => Texture {
                data: img.into_rgba8().into_raw(),
                width,
                height,
                channels: 4,
            },
            None => match img {
                DynamicImage::ImageLuma8(image_buffer) => Texture {
                    data: image_buffer.into_raw(),
                    width,
                    height,
                    channels: 1,
                },
                DynamicImage::ImageLumaA8(image_buffer) => Texture {
                    data: image_buffer.into_raw(),
                    width,
                    height,
                    channels: 2,
                },
                DynamicImage::ImageRgb8(image_buffer) => Texture {
                    data: image_buffer.into_raw(),
                    width,
                    height,
                    channels: 3,
                },
                DynamicImage::ImageRgba8(image_buffer) => Texture {
                    data: image_buffer.into_raw(),
                    width,
                    height,
                    channels: 4,
                },
                _ => Texture {
                    data: img.into_rgba8().into_raw(),
                    width,
                    height,
                    channels: 4,
                },
            },
            _ => panic!("Unsupported desired_channels value"),
        }
    }
    pub fn load(&mut self, filename: &std::path::Path, desired_channels: Option<u32>) {
        let img = image::ImageReader::open(filename)
            .expect("Failed to open image")
            .decode()
            .expect("Failed to decode image");
        let (width, height) = img.dimensions();
        match desired_channels {
            Some(1) => {
                self.data = img.into_luma8().into_raw();
                self.width = width;
                self.height = height;
                self.channels = 1;
            }
            Some(2) => {
                self.data = img.into_luma_alpha8().into_raw();
                self.width = width;
                self.height = height;
                self.channels = 2;
            }
            Some(3) => {
                self.data = img.into_rgb8().into_raw();
                self.width = width;
                self.height = height;
                self.channels = 3;
            }
            Some(4) => {
                self.data = img.into_rgba8().into_raw();
                self.width = width;
                self.height = height;
                self.channels = 4;
            }
            None => match img {
                DynamicImage::ImageLuma8(image_buffer) => {
                    self.data = image_buffer.into_raw();
                    self.width = width;
                    self.height = height;
                    self.channels = 1;
                }
                DynamicImage::ImageLumaA8(image_buffer) => {
                    self.data = image_buffer.into_raw();
                    self.width = width;
                    self.height = height;
                    self.channels = 2;
                }
                DynamicImage::ImageRgb8(image_buffer) => {
                    self.data = image_buffer.into_raw();
                    self.width = width;
                    self.height = height;
                    self.channels = 3;
                }
                DynamicImage::ImageRgba8(image_buffer) => {
                    self.data = image_buffer.into_raw();
                    self.width = width;
                    self.height = height;
                    self.channels = 4;
                }
                _ => {
                    self.data = img.into_rgba8().into_raw();
                    self.width = width;
                    self.height = height;
                    self.channels = 4;
                }
            },
            _ => panic!("Unsupported desired_channels value"),
        };
    }
    pub fn get_index(&self, x: i32, y: i32) -> i32 {
        self.channels as i32 * (self.width as i32 * (self.height as i32 - y - 1) + x)
    }
    pub fn get_luma(&self, u: f32, v: f32) -> f32 {
        // 为了保证clamp的结果正确必须使用带符号的类型，比如说-1需要clamp到1，用usize会clamp到width - 1
        let u = clamp(u * self.width as f32, 0.0, self.width as f32);
        let v = clamp(v * self.height as f32, 0.0, self.height as f32);
        let center_x = u.round() as i32;
        let center_y = v.round() as i32;
        let idx = [
            self.get_index(
                clamp(center_x - 1, 0, self.width as i32 - 1),
                clamp(center_y, 0, self.height as i32 - 1),
            ),
            self.get_index(
                clamp(center_x, 0, self.width as i32 - 1),
                clamp(center_y - 1, 0, self.height as i32 - 1),
            ),
            self.get_index(
                clamp(center_x - 1, 0, self.width as i32 - 1),
                clamp(center_y, 0, self.height as i32 - 1),
            ),
            self.get_index(
                clamp(center_x, 0, self.width as i32 - 1),
                clamp(center_y, 0, self.height as i32 - 1),
            ),
        ];
        let h_rate = u + 0.5 - center_x as f32;
        let v_rate = v + 0.5 - center_y as f32;
        let rgb_top = (1.0 - h_rate) * self.data[idx[0] as usize] as f32 / 255.0
            + h_rate * self.data[idx[1] as usize] as f32 / 255.0;
        let rgb_bottom = (1.0 - h_rate) * self.data[idx[2] as usize] as f32 / 255.0
            + h_rate * self.data[idx[3] as usize] as f32 / 255.0;
        (1.0 - v_rate) * rgb_bottom + v_rate * rgb_top
    }
    pub fn get_luma_alpha(&self, u: f32, v: f32) -> Vector2f {
        // 为了保证clamp的结果正确必须使用带符号的类型，比如说-1需要clamp到1，用usize会clamp到width - 1
        let u = clamp(u * self.width as f32, 0.0, self.width as f32);
        let v = clamp(v * self.height as f32, 0.0, self.height as f32);
        let center_x = u.round() as i32;
        let center_y = v.round() as i32;
        let idx = [
            self.get_index(
                clamp(center_x - 1, 0, self.width as i32 - 1),
                clamp(center_y, 0, self.height as i32 - 1),
            ),
            self.get_index(
                clamp(center_x, 0, self.width as i32 - 1),
                clamp(center_y - 1, 0, self.height as i32 - 1),
            ),
            self.get_index(
                clamp(center_x - 1, 0, self.width as i32 - 1),
                clamp(center_y, 0, self.height as i32 - 1),
            ),
            self.get_index(
                clamp(center_x, 0, self.width as i32 - 1),
                clamp(center_y, 0, self.height as i32 - 1),
            ),
        ];
        let h_rate = u + 0.5 - center_x as f32;
        let v_rate = v + 0.5 - center_y as f32;
        let rgb_top = (1.0 - h_rate)
            * Vector2f::new(
                self.data[idx[0] as usize] as f32 / 255.0,
                self.data[idx[0] as usize + 1] as f32 / 255.0,
            )
            + h_rate
                * Vector2f::new(
                    self.data[idx[1] as usize] as f32 / 255.0,
                    self.data[idx[1] as usize + 1] as f32 / 255.0,
                );
        let rgb_bottom = (1.0 - h_rate)
            * Vector2f::new(
                self.data[idx[2] as usize] as f32 / 255.0,
                self.data[idx[2] as usize + 1] as f32 / 255.0,
            )
            + h_rate
                * Vector2f::new(
                    self.data[idx[3] as usize] as f32 / 255.0,
                    self.data[idx[3] as usize + 1] as f32 / 255.0,
                );
        (1.0 - v_rate) * rgb_bottom + v_rate * rgb_top
    }

    pub fn get_rgb(&self, u: f32, v: f32) -> Vector3f {
        // 为了保证clamp的结果正确必须使用带符号的类型，比如说-1需要clamp到1，用usize会clamp到width - 1
        let u = clamp(u * self.width as f32, 0.0, self.width as f32);
        let v = clamp(v * self.height as f32, 0.0, self.height as f32);
        let center_x = u.round() as i32;
        let center_y = v.round() as i32;
        let idx = [
            self.get_index(
                clamp(center_x - 1, 0, self.width as i32 - 1),
                clamp(center_y, 0, self.height as i32 - 1),
            ),
            self.get_index(
                clamp(center_x, 0, self.width as i32 - 1),
                clamp(center_y - 1, 0, self.height as i32 - 1),
            ),
            self.get_index(
                clamp(center_x - 1, 0, self.width as i32 - 1),
                clamp(center_y, 0, self.height as i32 - 1),
            ),
            self.get_index(
                clamp(center_x, 0, self.width as i32 - 1),
                clamp(center_y, 0, self.height as i32 - 1),
            ),
        ];
        let h_rate = u + 0.5 - center_x as f32;
        let v_rate = v + 0.5 - center_y as f32;
        let rgb_top = (1.0 - h_rate)
            * Vector3f::new(
                self.data[idx[0] as usize] as f32 / 255.0,
                self.data[idx[0] as usize + 1] as f32 / 255.0,
                self.data[idx[0] as usize + 2] as f32 / 255.0,
            )
            + h_rate
                * Vector3f::new(
                    self.data[idx[1] as usize] as f32 / 255.0,
                    self.data[idx[1] as usize + 1] as f32 / 255.0,
                    self.data[idx[1] as usize + 2] as f32 / 255.0,
                );
        let rgb_bottom = (1.0 - h_rate)
            * Vector3f::new(
                self.data[idx[2] as usize] as f32 / 255.0,
                self.data[idx[2] as usize + 1] as f32 / 255.0,
                self.data[idx[2] as usize + 2] as f32 / 255.0,
            )
            + h_rate
                * Vector3f::new(
                    self.data[idx[3] as usize] as f32 / 255.0,
                    self.data[idx[3] as usize + 1] as f32 / 255.0,
                    self.data[idx[3] as usize + 2] as f32 / 255.0,
                );
        (1.0 - v_rate) * rgb_bottom + v_rate * rgb_top
    }
    pub fn get_rgba(&self, u: f32, v: f32) -> Vector4f {
        let u = clamp(u * self.width as f32, 0.0, self.width as f32);
        let v = clamp(v * self.height as f32, 0.0, self.height as f32);
        let center_x = u.round() as i32;
        let center_y = v.round() as i32;
        let idx = [
            self.get_index(
                clamp(center_x - 1, 0, self.width as i32 - 1),
                clamp(center_y, 0, self.height as i32 - 1),
            ),
            self.get_index(
                clamp(center_x, 0, self.width as i32 - 1),
                clamp(center_y - 1, 0, self.height as i32 - 1),
            ),
            self.get_index(
                clamp(center_x - 1, 0, self.width as i32 - 1),
                clamp(center_y, 0, self.height as i32 - 1),
            ),
            self.get_index(
                clamp(center_x, 0, self.width as i32 - 1),
                clamp(center_y, 0, self.height as i32 - 1),
            ),
        ];
        let h_rate = u + 0.5 - center_x as f32;
        let v_rate = v + 0.5 - center_y as f32;
        let rgba_top = (1.0 - h_rate)
            * Vector4f::new(
                self.data[idx[0] as usize] as f32 / 255.0,
                self.data[idx[0] as usize + 1] as f32 / 255.0,
                self.data[idx[0] as usize + 2] as f32 / 255.0,
                self.data[idx[0] as usize + 3] as f32 / 255.0,
            )
            + h_rate
                * Vector4f::new(
                    self.data[idx[1] as usize] as f32 / 255.0,
                    self.data[idx[1] as usize + 1] as f32 / 255.0,
                    self.data[idx[1] as usize + 2] as f32 / 255.0,
                    self.data[idx[1] as usize + 3] as f32 / 255.0,
                );
        let rgba_bottom = (1.0 - h_rate)
            * Vector4f::new(
                self.data[idx[2] as usize] as f32 / 255.0,
                self.data[idx[2] as usize + 1] as f32 / 255.0,
                self.data[idx[2] as usize + 2] as f32 / 255.0,
                self.data[idx[2] as usize + 3] as f32 / 255.0,
            )
            + h_rate
                * Vector4f::new(
                    self.data[idx[3] as usize] as f32 / 255.0,
                    self.data[idx[3] as usize + 1] as f32 / 255.0,
                    self.data[idx[3] as usize + 2] as f32 / 255.0,
                    self.data[idx[3] as usize + 3] as f32 / 255.0,
                );
        (1.0 - v_rate) * rgba_bottom + v_rate * rgba_top
    }
}
