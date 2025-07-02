/*!
This module provides a movable camera.

WASD keys to move the camera.
mouse to look around.
*/
use std::sync::{Arc, Mutex};
use std::f32::consts::PI;

const SPEED: f32 = 1.0;

pub struct CameraController {
    position: [f32; 3],
    direction: [f32; 3],
    moving_forward: bool,
    moving_backward: bool,
    moving_left: bool,
    moving_right: bool,

    yaw: f32, // 水平旋转角度
    pitch: f32, // 垂直旋转角度
    sensitivity: f32, // 鼠标灵敏度
    is_draging: bool,
    last_mouse_pos: (f32, f32),
    mouse_offset: (f32, f32), // 鼠标偏移量
}

#[derive(Debug, Clone)]
pub enum InputEvent {
    KeyPress(String),
    KeyRelease(String),
    MouseMove(f32, f32),
    MouseClick,
}

impl CameraController {
    pub fn new() -> Self {
        Self {
            position: [0.0, 0.0, 7.0],
            // 默认朝向Z轴负方向
            direction: [0.0, 0.0, -1.0],
            moving_forward: false,
            moving_backward: false,
            moving_left: false,
            moving_right: false,

            yaw: -PI / 2.0, // 正前方
            pitch: 0.0,
            sensitivity: 0.1,
            is_draging: false,
            last_mouse_pos: (0.0, 0.0),
            mouse_offset: (0.0, 0.0),
        }
    }

    pub fn handle_event(&mut self, event: &InputEvent) {
        match event {
            InputEvent::KeyPress(key) => match key.as_str() {
                "w" => {
                    self.moving_forward = true;
                },
                "a" => {
                    self.moving_left = true;
                },
                "s" => {
                    self.moving_backward = true;
                },
                "d" => {
                    self.moving_right = true;
                },
                "q" => {
                    self.moving_forward = true;
                    self.moving_left = true;
                },
                "e" => {
                    self.moving_forward = true;
                    self.moving_right = true;
                },
                _ => {}
            },
            InputEvent::KeyRelease(key) => match key.as_str() {
                "w" => {
                    self.moving_forward = false;
                },
                "a" => {
                    self.moving_left = false;
                },
                "s" => {
                    self.moving_backward = false;
                },
                "d" => {
                    self.moving_right = false;
                },
                "q" => {
                    self.moving_forward = false;
                    self.moving_left = false;
                },
                "e" => {
                    self.moving_forward = false;
                    self.moving_right = false;
                }
                _ => {}
            }
            InputEvent::MouseMove(x, y) => {
                self.mouse_offset.0 = x - self.last_mouse_pos.0;
                self.mouse_offset.1 = y - self.last_mouse_pos.1;

                self.last_mouse_pos.0 = *x;
                self.last_mouse_pos.1 = *y;
            }
            InputEvent::MouseClick => {
                // 处理鼠标点击事件
                self.is_draging = true;
            }
        }
    }

    pub fn update(&mut self) {
        if self.moving_forward {
            self.position[0] += self.direction[0] * SPEED;
            self.position[1] += self.direction[1] * SPEED;
            self.position[2] += self.direction[2] * SPEED;
        }
        if self.moving_left {
            self.position[0] += self.direction[2] * SPEED;
            self.position[2] -= self.direction[0] * SPEED;
            // y = 0
        }
        if self.moving_backward {
            self.position[0] -= self.direction[0] * SPEED;
            self.position[1] -= self.direction[1] * SPEED;
            self.position[2] -= self.direction[2] * SPEED;
        }
        if self.moving_right {
            self.position[0] -= self.direction[2] * SPEED;
            self.position[2] += self.direction[0] * SPEED;
            // y = 0
        }
        if self.is_draging {
            // 旋转视角
            self.yaw += self.mouse_offset.0 * self.sensitivity;
            self.pitch -= self.mouse_offset.1 * self.sensitivity;

            // 限制俯仰角，防止摄像机翻转
            let max_pitch = PI / 2.0 - 0.01; // 约89°
            self.pitch = self.pitch.clamp(-max_pitch, max_pitch);

            // 更新方向向量
            self.direction[0] = self.yaw.cos() * self.pitch.cos();
            self.direction[1] = self.pitch.sin();
            self.direction[2] = self.yaw.sin() * self.pitch.cos();
            // println!("offset: {}, {}", self.mouse_offset.0, self.mouse_offset.1);
            // println!("Direction: {:?}", self.direction);
            // println!("rad: raw {}, pitch {}", self.yaw, self.pitch);
            self.is_draging = false;
        }
    }

    pub fn get_pos(&self) -> [f32; 3] {
        self.position
    }

    pub fn get_dir(&self) -> [f32; 3] {
        self.direction
    }

    pub fn set_dir(&mut self, dir: [f32; 3]) {
        self.direction = dir;
    }
}