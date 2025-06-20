/*!
This module provides a movable camera.

WASD keys to move the camera.
mouse to look around.
*/
#![allow(unused)]
use std::sync::{Arc, Mutex};

pub struct CameraController {
    position: [u32; 3],
    rotation: [f32; 3],
    moving_up: bool,
    moving_down: bool,
    moving_left: bool,
    moving_right: bool,
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
            position: [0, 0, 0],
            rotation: [0.0, 0.0, 0.0],
            moving_up: false,
            moving_down: false,
            moving_left: false,
            moving_right: false,
        }
    }

    pub fn handle_key_event(&mut self) {
        
    }

    pub fn handle_mouse_motion(&mut self, delta_x: f64, delta_y: f64) {
        // 根据鼠标移动更新摄像机方向

    }

    pub fn handle_event(&mut self, event: &InputEvent) {
        match event {
            InputEvent::KeyPress(key) => match key.as_str() {
                "w" => {
                    self.moving_up = true;
                },
                "a" => {
                    self.moving_left = true;
                },
                "s" => {
                    self.moving_down = true;
                },
                "d" => {
                    self.moving_right = true;
                },
                "q" => {
                    self.moving_up = true;
                    self.moving_left = true;
                },
                "e" => {
                    self.moving_up = true;
                    self.moving_right = true;
                },
                _ => {}
            },
            InputEvent::KeyRelease(key) => match key.as_str() {
                "w" => {
                    self.moving_up = false;
                },
                "a" => {
                    self.moving_left = false;
                },
                "s" => {
                    self.moving_down = false;
                },
                "d" => {
                    self.moving_right = false;
                },
                "q" => {
                    self.moving_up = false;
                    self.moving_left = false;
                },
                "e" => {
                    self.moving_up = false;
                    self.moving_right = false;
                }
                _ => {}
            }
            InputEvent::MouseMove(dx, dy) => {
                // 处理鼠标移动，例如旋转视角

            }
            InputEvent::MouseClick => {
                // 处理鼠标点击事件
            }
        }
    }

    pub fn update(&mut self) {
        if self.moving_up {
            self.position[1] = self.position[1].wrapping_sub(10) % 600;
        }
        if self.moving_left {
            self.position[0] = self.position[0].wrapping_sub(10) % 700;
        }
        if self.moving_down {
            self.position[1] = self.position[1].wrapping_add(10) % 600;
        }
        if self.moving_right {
            self.position[0] = self.position[0].wrapping_add(10) % 700;
        }
    }

    pub fn get_pos(&self) -> [u32; 3] {
        self.position
    }
}