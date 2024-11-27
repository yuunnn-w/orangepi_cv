# YOLO Object Detection Server for OrangePi 5 Max
<p align="left">
    <a href="README_CN.md">中文</a>&nbsp ｜ &nbspEnglish&nbsp
</p>

## Overview

This project implements a YOLO object detection server that runs on the OrangePi 5 Max development board. The server is designed for real-time object detection of low-altitude drones and includes features such as servo control for real-time tracking, dual-mode image processing for both thermal infrared and visible light, and remote access via a WebSocket server. The project is built using C++ and supports cross-platform compilation with CMake.

## Features

1. **Real-time Object Detection for Low-altitude Drones**: The server is optimized for detecting drones in low-altitude environments.
2. **Servo Control for Real-time Tracking**: Utilizes GPIO interfaces or USB serial ports to control servos for tracking detected objects.
3. **Dual-mode Image Processing**: Processes both thermal infrared and visible light images to adapt to night and day environments.
4. **WebSocket Server**: Implements a WebSocket server using uWebSockets, allowing remote access and control from a cloud host. Features include hot-reloading detection models, switching operation modes, and retrieving the current detection frame.
5. **Queue-based Detection Loop**: Implements a queue-based detection loop with separate threads for image acquisition, detection, and post-processing, enhancing frame rate and inference performance.
6. **OrangePi 5 Max Development Board**: Built for the OrangePi 5 Max, which features the RK3588 CPU with three 2Tops int8 NPU cores, supporting quantization and mixed-precision inference.
7. **UVC Protocol for USB Cameras**: Controls USB cameras using the UVC protocol, supporting features like mode switching.
8. **Cross-platform Build with CMake**: The project supports cross-platform compilation using CMake and is written in pure C++ for high performance and low latency.

## License

This project is licensed under the GPL-3.0 License.

## Prerequisites

Before you begin, ensure you have met the following requirements:

- **OrangePi 5 Max Development Board**: The project is designed to run on the OrangePi 5 Max.
- **Operating System**: A compatible Linux distribution (e.g., Ubuntu, Debian).
- **Dependencies**:
  - CMake (>= 3.10)
  - GCC or Clang with C++17 support
  - OpenCV (>= 4.0)
  - uWebSockets
  - libuvc (for UVC camera control)
  - WiringPi or similar GPIO library (for servo control)

## Installation

### Step 1: Install Dependencies

First, install the necessary dependencies:

```bash
sudo apt-get update
sudo apt-get install -y build-essential gcc g++ cmake git libopencv-dev libuvc-dev zlib1g-dev
# wiringpi need to install from another way.
```
For OrangePi you should install [wiringOP](https://github.com/orangepi-xunlong/wiringOP).
You can download [wiringpi_2.57.deb](https://github.com/orangepi-xunlong/orangepi-build/blob/next/external/cache/debs/arm64/wiringpi_2.57.deb) to install directly by `dpkg`.

### Step 2: Clone the Repository

Clone the project repository to your local machine:

```bash
git clone https://github.com/yuunnn-w/orangepi_cv.git
cd orangepi_cv
```

### Step 3: Install uWebSockets

The project uses uWebSockets for the WebSocket server. Follow these steps to install uWebSockets:

1. **Clone the uWebSockets Repository**:

   ```bash
   git clone https://github.com/uNetworking/uWebSockets.git
   cd uWebSockets
   git submodule update --init  --recursive
   ```

2. **Build and Install uWebSockets**:

   ```bash
   make
   sudo make install
   sudo cp uSockets/src/libusockets.h /usr/local/include/uWebSockets/
   sudo mv uSockets/uSockets.a /usr/lib/aarch64-linux-gnu/libuSockets.a
   cd ..
   ```
### Step 4: Build the Project

Create a build directory and compile the project using CMake:

```bash
mkdir build
cd build
cmake ..
make
```

### Step 5: Run the Server

After building the project, you can run the YOLO object detection server:

```bash
./orangepi_cv
```

## Usage

### WebSocket Server

The WebSocket server listens on a default port (e.g., 9001). You can connect to it using a WebSocket client to perform the following actions:

- **Hot-reload Detection Model**: Send a message to reload the detection model.
- **Switch Operation Mode**: Change between thermal infrared and visible light modes.
- **Retrieve Current Detection Frame**: Request the current detection frame.

### Servo Control

The server can control servos via GPIO or USB serial ports. Ensure that the GPIO pins are correctly configured and that the servos are connected as per the project's wiring diagram.

### Camera Control

The server uses UVC protocol to control USB cameras. Ensure that the cameras are connected and recognized by the system. You can switch between different camera modes (e.g., thermal, visible) via the WebSocket server.

## Contributing

Contributions are welcome! Please read the [CONTRIBUTING.md](CONTRIBUTING.md) file for guidelines on how to contribute to this project.

## License

This project is licensed under the GPL-3.0 License. See the [LICENSE](LICENSE) file for details.

