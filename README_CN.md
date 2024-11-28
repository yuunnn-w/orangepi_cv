# OrangePi 5 Max YOLO 目标检测服务器

<p align="left">
    中文&nbsp ｜ &nbsp<a href="README.md">English</a>
</p>

## 概述

本项目实现了一个在 OrangePi 5 Max 开发板上运行的 YOLO 目标检测服务器。该服务器专为低空无人机的实时目标检测而设计，并具备舵机控制实时跟踪、红外与可见光双模图像处理、以及通过 WebSocket 服务器进行远程访问等功能。项目使用 C++ 编写，支持跨平台编译，采用 CMake 构建系统。

## 功能特性

1. **低空无人机实时目标检测**：服务器针对低空环境中的无人机检测进行了优化。
2. **舵机控制实时跟踪**：利用 GPIO 接口或 USB 串口控制舵机，实现对检测目标的实时跟踪。
3. **红外与可见光双模图像处理**：处理红外与可见光图像，适应昼夜环境。
4. **WebSocket 服务器**：使用 uWebSockets 实现 WebSocket 服务器，支持从云主机进行远程访问和控制。功能包括热加载检测模型、切换操作模式、获取当前检测帧等。
5. **基于队列的检测循环**：实现基于队列的检测循环，使用独立线程进行图像采集、检测和后处理，提升帧率和推理性能。
6. **OrangePi 5 Max 开发板**：专为 OrangePi 5 Max 设计，该开发板搭载 RK3588 CPU，具备三个 2Tops int8 NPU 核心，支持量化和混合精度推理。
7. **UVC 协议控制 USB 摄像头**：使用 UVC 协议控制 USB 摄像头，支持模式切换等功能。
8. **跨平台构建与 CMake**：项目支持跨平台编译，使用 CMake 构建系统，采用纯 C++ 编写，确保高性能和低延迟。

## 许可证

本项目采用 GPL-3.0 许可证。

## 先决条件

在开始之前，请确保您已满足以下要求：

- **OrangePi 5 Max 开发板**：项目专为 OrangePi 5 Max 设计，请在开发板端进行编译。
- **操作系统**：兼容的 Linux 发行版（例如，Ubuntu、Debian）。
- **依赖项**：
  - CMake (>= 3.10)
  - 支持 C++17 的 GCC 或 Clang
  - OpenCV (>= 4.0)
  - uWebSockets
  - libuvc（用于 UVC 摄像头控制）
  - WiringPi 或其他类似 GPIO 库（用于舵机控制）

## 安装步骤

### 第一步：安装依赖项

首先，安装必要的依赖项：

```bash
sudo apt-get update
sudo apt-get install -y build-essential gcc g++ cmake ninja-build git libopencv-dev libuvc-dev libusb-1.0-0-dev zlib1g-dev
# wiringpi 需要通过其他方式安装。
```
对于 OrangePi，您需要安装 [wiringOP](https://github.com/orangepi-xunlong/wiringOP)。

### 第二步：克隆仓库

将项目仓库克隆到本地机器：

```bash
git clone https://github.com/yuunnn-w/orangepi_cv.git
cd orangepi_cv
```

### 第三步：安装 uWebSockets

项目使用 uWebSockets 实现 WebSocket 服务器。按照以下步骤安装 uWebSockets：

1. **克隆 uWebSockets 仓库**：

   ```bash
   git clone https://github.com/uNetworking/uWebSockets.git
   cd uWebSockets
   git submodule update --init  --recursive
   ```

2. **构建并安装 uWebSockets**：

   ```bash
   make
   sudo make install
   sudo cp uSockets/src/libusockets.h /usr/local/include/uWebSockets/
   sudo mv uSockets/uSockets.a /usr/lib/aarch64-linux-gnu/libuSockets.a
   cd ..
   ```

### 第四步：构建项目

创建构建目录并使用 CMake 编译项目：

```bash
mkdir build
cd build
cmake ..
make
```

### 第五步：运行服务器

项目构建完成后，您可以运行 YOLO 目标检测服务器：

```bash
./orangepi_cv
```

## 使用方法

### WebSocket 服务器

WebSocket 服务器默认监听端口（例如 9001）。您可以使用 WebSocket 客户端连接到服务器，执行以下操作：

- **热加载检测模型**：发送消息以重新加载检测模型。
- **切换操作模式**：在红外与可见光模式之间切换。
- **获取当前检测帧**：请求当前检测帧。

### 舵机控制

服务器可以通过 GPIO 或 USB 串口控制舵机。确保 GPIO 引脚已正确配置，并且舵机按项目接线图连接。

### 摄像头控制

服务器使用 UVC 协议控制 USB 摄像头。确保摄像头已连接并被系统识别。您可以通过 WebSocket 服务器在不同摄像头模式（例如，红外、可见光）之间切换。

## 贡献

欢迎贡献！请阅读 [CONTRIBUTING.md](CONTRIBUTING.md) 文件，了解如何为该项目贡献代码。

## 许可证

本项目采用 GPL-3.0 许可证。详细信息请参阅 [LICENSE](LICENSE) 文件。
