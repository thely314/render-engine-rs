render-engine-rs
---

A hybrid rendering library implementing both rasterization and ray-tracing techniques, with a demonstration application featuring dynamic camera control and model visualization.

## Features
### Core Library (`render-lib`)
- **Rasterization Pipeline**
  - Triangle-based rendering
  - Z-buffer depth testing
  - Basic texture mapping
- **Ray Tracing**
  - BVH acceleration structure
  - Physically Based Rendering (PBR) materials
  - Monte Carlo path tracing
- **Common Infrastructure**
  - Math library (vectors, matrices, transforms)
  - Scene graph implementation
  - Material system

### Application (`render-app`)
- 🎮 Interactive viewer with camera controls
- 🧊 OBJ/GLTF model loading

## Getting Started

### Prerequisites
- Rust 1.70+
- TODO

### Installation
```bash
git clone https://github.com/thely314/render-engine-rs.git
cd render-engine-rs

# Build all components
cargo build --workspace --release

# Run the demo application
cargo run -p render-app --release -- --model assets/models/cornell_box.obj
TODO
```

## Usage
### Library Integration
Add to your Cargo.toml:
```toml
[dependencies]
render-lib = { git = "https://github.com/thely314/render-engine-rs" }
```

### Application Controls
|Key|Action|
|:-:|:-:|
|WASD|Camera movement|
|Mouse Drag|Orientation control|

## Documentation
Generate local docs:
```bash
cargo doc --workspace --no-deps --open
```

## License
Distributed under the MIT License.

## Contribution Agreement
By submitting commits to this repository, you agree to authorize your contribution under the MIT license.