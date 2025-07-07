render-engine-rs
---

2025 年 Rust 课程大作业

一个实现 3D 模型光栅化的渲染库，附带一个演示 demo，具有摄像机控制与添加光源、更换渲染模型功能。

注意：只在 `Windows 11 x64` 平台进行了测试

## 使用方式

### 前提条件
- Rust 1.70+ with MSVC Toolchain
- CMake 3.20+, support Visual Studio Toolchain

### 安装方式
```bash
git clone https://github.com/thely314/render-engine-rs.git
cd render-engine-rs

# Build all components 构建所有组件
cargo build --workspace --release

# Run the demo application 运行示例 demo
cargo run -p render-app --release
```

### 直接使用 lib
在 Cargo.toml 添加:
```toml
[dependencies]
render-lib = { git = "https://github.com/thely314/render-engine-rs" }
```

## 项目说明
### 核心 lib (`render-lib`)
- 借助线性代数库 `nalgebra` 加快计算
- 通过 `assimp` 导入模型文件

### 示例 demo (`render-app`)
- 可视化摄像机控制
- 模型热加载
- 使用WASD控制摄像机移动，按住鼠标左键拖动控制摄像机视角旋转

## 本地文档生成
```bash
cargo doc --workspace --no-deps --open
```

## License
Distributed under the MIT License.

按照 MIT License 分发

## Contribution Agreement 贡献需知
By submitting commits to this repository, you agree to authorize your contribution under the MIT license.

通过向本仓库提交 commits，贡献者同意根据 MIT license 授权其贡献。