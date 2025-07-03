/*!
This module provides data struct of lights.
*/
#![allow(unused)]


use std::sync::{Arc, Mutex};
use std::collections::HashMap;
use render_lib::rasterization::directional_light;
use render_lib::rasterization::{
    light::LightTrait,
    spot_light::SpotLight,
    directional_light::DirectionalLight,
};
use render_lib::util::math::*;

#[derive(Debug, Clone)]
pub struct LightIdentifier {
    pub name: slint::SharedString,
    pub light_type: slint::SharedString,
    pub position_x: f32,
    pub position_y: f32,
    pub position_z: f32,
    pub color_r: f32,
    pub color_g: f32,
    pub color_b: f32,
    pub forward_x: f32,
    pub forward_y: f32,
    pub forward_z: f32,
}

impl LightIdentifier {
    pub fn get_attribute(&mut self) -> (slint::SharedString, slint::SharedString, f32, f32, f32, f32, f32, f32, f32, f32, f32) {
        (
            self.name.clone(),
            self.light_type.clone(),
            self.position_x,
            self.position_y,
            self.position_z,
            self.color_r,
            self.color_g,
            self.color_b,
            self.forward_x,
            self.forward_y,
            self.forward_z,
        )
    }
}

pub struct LightManager {
    pub lights: HashMap<String, Arc<Mutex<dyn LightTrait>>>,
}

impl LightManager {
    pub fn new() -> Self {
        LightManager {
            lights: HashMap::new(),
        }
    }

    /// return `Arc<Mutex<SpotLight>>` need to be manual add to scene
    pub fn add_spot_light(&mut self, light: LightIdentifier) ->Arc<Mutex<SpotLight>> {
        let spot_light = Arc::new(Mutex::new(SpotLight::default()));
        spot_light.lock().unwrap().set_pos(Vector3f::new(light.position_x, light.position_y, light.position_z));
        spot_light.lock().unwrap().set_intensity(Vector3f::new(light.color_r, light.color_g, light.color_b));
        spot_light.lock().unwrap().set_light_dir(Vector3f::new(light.forward_x, light.forward_y, light.forward_z).normalize());
        self.lights.insert(light.name.to_string(), spot_light.clone());
        spot_light
    }

    /// return `Arc<Mutex<DirectionalLight>>` need to be manual add to scene
    pub fn add_directional_light(&mut self, light: LightIdentifier) -> Arc<Mutex<DirectionalLight>> {
        let directional_light = Arc::new(Mutex::new(DirectionalLight::default()));
        // directional_light.lock().unwrap().set_shadow_status(false);
        directional_light.lock().unwrap().set_pos(Vector3f::new(light.position_x, light.position_y, light.position_z));
        directional_light.lock().unwrap().set_intensity(Vector3f::new(light.color_r, light.color_g, light.color_b));
        directional_light.lock().unwrap().set_light_dir((Vector3f::new(light.forward_x, light.forward_y, light.forward_z)).normalize());
        self.lights.insert(light.name.to_string(), directional_light.clone());
        directional_light
    }

    /// return `Option<Arc<Mutex<dyn LightTrait>>>` need to be manual delete from scene
    pub fn delete_light(&mut self, light: LightIdentifier) -> Option<Arc<Mutex<dyn LightTrait>>> {
        if let Some(light_arc) = self.lights.remove(light.name.as_str()) {
            Some(light_arc)
        } else {
            println!("Target not exists");
            None
        }
    }
}