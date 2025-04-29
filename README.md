# OrangePi 5 Max YOLO Object Detection Server

<p align="left">
    <a href="README_CN.md">中文</a>&nbsp ｜ &nbspEnglish&nbsp
</p>

This project implements a YOLO object detection and tracking server running on the OrangePi 5 Max development board. The server is specifically designed for real-time tracking of low-altitude drones, featuring servo-controlled real-time tracking, dual-mode (thermal infrared and visible light) image processing, remote access and control via network, and support for expansion into a multi-station array. The project is written in C++, supports cross-platform compilation, and uses the CMake build system.

## Features

1. **Real-time Object Detection**: Uses YOLO model for object detection with customizable target types, achieving 30 FPS detection speed.
2. **Dual-mode Image Processing**: Supports real-time processing and switching between thermal infrared and visible light images.
3. **Servo Control**: Implements real-time target tracking using Johor bus servos, with Kalman filtering to smooth and predict target positions, reducing latency.
4. **Network Access**: Enables remote access and control via TCP/IP protocol.
5. **Multi-station Array**: Supports expansion into multiple base stations for collaborative tracking, improving accuracy and coverage.
6. **Thread Safety**: Uses thread-safe queues for data transmission to ensure consistency in multi-threaded environments.
7. **Cross-platform Support**: Built with CMake for compatibility across multiple platforms.
8. **OrangePi 5 Max Optimization**: Specifically optimized for the OrangePi 5 Max development board to maximize hardware performance.
9. **RKNN Model Support**: Utilizes RKNN models for efficient object detection.
10. **UVC Protocol Support**: Compatible with UVC protocol USB cameras and thermal imagers.

## License

This project is licensed under the GPL-3.0 License.

## Prerequisites

Before you begin, ensure you meet the following requirements:

- **OrangePi 5 Max Development Board**: The project is designed for the OrangePi 5 Max. Compile and run on the board.
- **Operating System**: Compatible Linux distribution (e.g., Ubuntu, Debian).
- **Dependencies**:
  - CMake (>= 3.10)
  - OpenCV (>= 4.5)
  - RKNN SDK
  - libuvc
  - WiringPi
  - See installation steps for detailed dependencies.

## Installation Steps

### Step 1: Install Dependencies

First, install the necessary dependencies:

```bash
sudo add-apt-repository ppa:jjriek/rockchip-multimedia  # Add Rockchip multimedia PPA. If not added, manually install librga-dev.
sudo apt-get update
sudo apt-get install -y build-essential gcc g++ cmake ninja-build git libopencv-dev libuvc-dev libusb-1.0-0-dev zlib1g-dev librga-dev ninja-build gdb nlohmann-json3-dev libeigen3-dev libtbb-dev
# wiringpi requires separate installation
```

For OrangePi 5 Max, install [wiringOP](https://github.com/orangepi-xunlong/wiringOP).

### Step 2: Clone the Repository

```bash
git clone https://github.com/yuunnn-w/orangepi_cv.git
cd orangepi_cv
```

### Step 3: Modify Code for Your Camera's VID and PID

In `main.cpp`, locate the following lines:

```cpp
uint16_t vid = 0x0bdc;  // Vendor ID (use lsusb to check)
uint16_t pid = 0x0678;  // Product ID
```

Replace `vid` and `pid` with your camera's actual vendor and product IDs. Use the `lsusb` command to find this information.

### Step 4: Modify Servo Driver Code for Your Servo Model

In `servo_driver.h`, locate the following lines:

```cpp
#define SERVO_ANGLE_RANGE 360.0       // Maximum servo rotation angle
#define SERVO_ANGLE_HALF_RANGE 180.0  // Half of the maximum angle
```

Replace `SERVO_ANGLE_RANGE` and `SERVO_ANGLE_HALF_RANGE` with your servo's actual angle range. Adjust according to your servo model.  
**Note**: This project uses Johor bus servos (official website: [https://johorobot.com/](https://johorobot.com/)). Other servo models are not currently supported.

### Step 5: Compile the Project

Create a build directory and compile the project with CMake:

```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

### Step 6: Run the Project

Before running, ensure all devices are connected to your OrangePi 5 Max, including:

- USB camera
- Thermal imager
- Servo
- Other sensors (if applicable)

After confirming the board is connected to the local network, start the server:

```bash
./orangepi_cv
```

### Step 7: Access the Server

On a Windows PC connected to the same network, navigate to the `python` folder in the project directory and run:

```bash
python3 ui.py
```

The client is written in PyQt5. Before running, ensure Python3 and PyQt5 are installed:

```bash
pip install numpy opencv-python pyqt5 matplotlib pynput
```

### Step 8: Usage Instructions

1. **Start the Server**: Run `./orangepi_cv` on the OrangePi 5 Max.
2. **Start the Client**: Run `python3 ui.py` on the Windows PC.
3. **Connect to Server**: The client automatically detects online base stations. Select a station and click "Start Video" to view real-time detection.
4. **Servo Control**: Enable the servo remote switch in the client, then use the `W`, `A`, `S`, and `D` keys to control servo movement.
5. **Capture Frames**: Click "Get Current Frame" to save the current detected image locally.
6. **Real-time Situational Display**: Click "Real-time Situational Map" to open a window showing detected target positions.
7. **System Sleep**: Click "System Sleep" to pause detection. Click "System Wake" to resume.
8. **System Shutdown**: Click "System Shutdown" to exit the program (irreversible; use with caution).

## Contributing

Contributions are welcome! Please read [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## License

This project is licensed under the GPL-3.0 License. See [LICENSE](LICENSE) for details.  
**Note**: GPL-3.0 allows code modification and secondary development, but you must open-source your code and credit the original author and source.

## Contact

For questions or suggestions, contact:

- **Maintainer**: Xiaoyang
- **Email**: [jiaxinsugar@gmail.com](mailto:jiaxinsugar@gmail.com)
- **GitHub**: [yuunnn-w](https://github.com/yuunnn-w)