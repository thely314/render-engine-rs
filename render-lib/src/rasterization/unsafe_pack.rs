use crate::rasterization::light::*;
use crate::rasterization::model::*;
use crate::rasterization::scene::*;

use super::directional_light::DirectionalLight;
use super::spot_light::SpotLight;
#[derive(Clone, Copy)]
pub(in crate::rasterization) struct MutScenePtr(pub(in crate::rasterization) *mut Scene);
unsafe impl Send for MutScenePtr {}
impl MutScenePtr {
    pub(in crate::rasterization) unsafe fn get_mut(self) -> *mut Scene {
        self.0
    }
    pub(in crate::rasterization) unsafe fn get(self) -> *const Scene {
        self.0
    }
}

#[derive(Clone, Copy)]
pub(in crate::rasterization) struct ConstScenePtr(pub(in crate::rasterization) *const Scene);
unsafe impl Send for ConstScenePtr {}
impl ConstScenePtr {
    pub(in crate::rasterization) unsafe fn get(self) -> *const Scene {
        self.0
    }
}

#[derive(Clone, Copy)]
pub(in crate::rasterization) struct MutSpotLightPtr(pub(in crate::rasterization) *mut SpotLight);
unsafe impl Send for MutSpotLightPtr {}
impl MutSpotLightPtr {
    pub(in crate::rasterization) unsafe fn get_mut(self) -> *mut SpotLight {
        self.0
    }
    pub(in crate::rasterization) unsafe fn get(self) -> *const SpotLight {
        self.0
    }
}

#[derive(Clone, Copy)]
pub(in crate::rasterization) struct ConstSpotLightPtr(
    pub(in crate::rasterization) *const SpotLight,
);
unsafe impl Send for ConstSpotLightPtr {}
impl ConstSpotLightPtr {
    pub(in crate::rasterization) unsafe fn get(self) -> *const SpotLight {
        self.0
    }
}

#[derive(Clone, Copy)]
pub(in crate::rasterization) struct MutDirectionalLightPtr(
    pub(in crate::rasterization) *mut DirectionalLight,
);
unsafe impl Send for MutDirectionalLightPtr {}
impl MutDirectionalLightPtr {
    pub(in crate::rasterization) unsafe fn get_mut(self) -> *mut DirectionalLight {
        self.0
    }
    pub(in crate::rasterization) unsafe fn get(self) -> *const DirectionalLight {
        self.0
    }
}

#[derive(Clone, Copy)]
pub(in crate::rasterization) struct ConstDirectionalLightPtr(
    pub(in crate::rasterization) *const DirectionalLight,
);
unsafe impl Send for ConstDirectionalLightPtr {}
impl ConstDirectionalLightPtr {
    pub(in crate::rasterization) unsafe fn get(self) -> *const DirectionalLight {
        self.0
    }
}

#[derive(Clone, Copy)]
pub(in crate::rasterization) struct ConstVecConstModelPtr(
    pub(in crate::rasterization) *const Vec<*const Model>,
);
unsafe impl Send for ConstVecConstModelPtr {}
impl ConstVecConstModelPtr {
    pub(in crate::rasterization) unsafe fn get(self) -> *const Vec<*const Model> {
        self.0
    }
}

#[derive(Clone, Copy)]
pub(in crate::rasterization) struct ConstVecMutModelPtr(
    pub(in crate::rasterization) *const Vec<*mut Model>,
);
unsafe impl Send for ConstVecMutModelPtr {}
impl ConstVecMutModelPtr {
    pub(in crate::rasterization) unsafe fn get(self) -> *const Vec<*mut Model> {
        self.0
    }
}

#[derive(Clone, Copy)]
pub(in crate::rasterization) struct MutVecConstModelPtr(
    pub(in crate::rasterization) *mut Vec<*const Model>,
);
unsafe impl Send for MutVecConstModelPtr {}
impl MutVecConstModelPtr {
    pub(in crate::rasterization) unsafe fn get_mut(self) -> *mut Vec<*const Model> {
        self.0
    }
    pub(in crate::rasterization) unsafe fn get(self) -> *const Vec<*const Model> {
        self.0
    }
}

#[derive(Clone, Copy)]
pub(in crate::rasterization) struct MutVecMutModelPtr(
    pub(in crate::rasterization) *mut Vec<*mut Model>,
);
unsafe impl Send for MutVecMutModelPtr {}
impl MutVecMutModelPtr {
    pub(in crate::rasterization) unsafe fn get_mut(self) -> *mut Vec<*mut Model> {
        self.0
    }
    pub(in crate::rasterization) unsafe fn get(self) -> *const Vec<*mut Model> {
        self.0
    }
}

pub(in crate::rasterization) struct ConstVecConstLightPtr(
    pub(in crate::rasterization) *const Vec<*const dyn LightTrait>,
);
unsafe impl Send for ConstVecConstLightPtr {}
impl ConstVecConstLightPtr {
    pub(in crate::rasterization) unsafe fn get(self) -> *const Vec<*const dyn LightTrait> {
        self.0
    }
}

#[derive(Clone, Copy)]
pub(in crate::rasterization) struct ConstVecMutLightPtr(
    pub(in crate::rasterization) *const Vec<*mut dyn LightTrait>,
);
unsafe impl Send for ConstVecMutLightPtr {}
impl ConstVecMutLightPtr {
    pub(in crate::rasterization) unsafe fn get(self) -> *const Vec<*mut dyn LightTrait> {
        self.0
    }
}

#[derive(Clone, Copy)]
pub(in crate::rasterization) struct MutVecConstLightPtr(
    pub(in crate::rasterization) *mut Vec<*const dyn LightTrait>,
);
unsafe impl Send for MutVecConstLightPtr {}
impl MutVecConstLightPtr {
    pub(in crate::rasterization) unsafe fn get_mut(self) -> *mut Vec<*const dyn LightTrait> {
        self.0
    }
    pub(in crate::rasterization) unsafe fn get(self) -> *const Vec<*const dyn LightTrait> {
        self.0
    }
}

#[derive(Clone, Copy)]
pub(in crate::rasterization) struct MutVecMutLightPtr(
    pub(in crate::rasterization) *mut Vec<*mut dyn LightTrait>,
);
unsafe impl Send for MutVecMutLightPtr {}
impl MutVecMutLightPtr {
    pub(in crate::rasterization) unsafe fn get_mut(self) -> *mut Vec<*mut dyn LightTrait> {
        self.0
    }
    pub(in crate::rasterization) unsafe fn get(self) -> *const Vec<*mut dyn LightTrait> {
        self.0
    }
}

#[derive(Clone, Copy)]
pub(in crate::rasterization) struct ZBufferPtr(pub(in crate::rasterization) *mut Vec<f32>);
unsafe impl Send for ZBufferPtr {}
impl ZBufferPtr {
    pub(in crate::rasterization) unsafe fn get_mut(self) -> *mut Vec<f32> {
        self.0
    }
    pub(in crate::rasterization) unsafe fn get(self) -> *const Vec<f32> {
        self.0
    }
}
