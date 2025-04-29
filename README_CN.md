# OrangePi 5 Max YOLO 目标检测服务器

<p align="left">
    中文&nbsp ｜ &nbsp<a href="README.md">English</a>
</p>

本项目实现了一个在 OrangePi 5 Max 开发板上运行的 YOLO 目标检测跟踪服务器。该服务器专为低空无人机的实时跟踪而设计，并具备舵机控制实时跟踪、红外与可见光双模图像处理、以及通过网络进行远程访问和控制等功能，且支持扩展为多基站构成阵列。项目使用 C++ 编写，支持跨平台编译，采用 CMake 构建系统。

## 功能特性

1. 实时目标检测：使用 YOLO 模型进行目标检测，支持自定义目标类型，检测速度达到 30 FPS。
2. 双模图像处理：支持热红外和可见光图像的实时处理与切换。
3. 舵机控制：通过炬晖总线舵机实现对目标的实时跟踪，采用Kalman滤波器平滑并预测目标位置，降低延迟。
4. 网络访问：通过 TCP / IP 协议实现远程访问和控制。
6. 多基站阵列：支持扩展为多基站，多个基站可以协同工作，有效提高跟踪的精度和范围。
7. 线程安全：使用线程安全队列实现数据传输，确保多线程环境下的数据一致性。
8. 跨平台支持：使用 CMake 构建系统，支持多种平台的编译与运行。
9. OrangePi 5 Max 开发板支持：专为 OrangePi 5 Max 开发板优化，充分利用其硬件性能。
10. RKNN模型支持：使用 RKNN 模型进行高效的目标检测。
11. UVC 协议支持：支持 UVC 协议的 USB 摄像头和热成像仪。

## 许可证

本项目采用 GPL-3.0 许可证。

## 先决条件

在开始之前，请确保您已满足以下要求：

- **OrangePi 5 Max 开发板**：项目专为 OrangePi 5 Max 设计，请在开发板端进行编译。
- **操作系统**：兼容的 Linux 发行版（例如，Ubuntu、Debian）。
- **依赖项**：
  - CMake (>= 3.10)
  - OpenCV (>= 4.5)
  - RKNN SDK
  - libuvc
  - WiringPi
  - 详细依赖见安装步骤

## 安装步骤

### 第一步：安装依赖项

首先，安装必要的依赖项：

```bash
sudo add-apt-repository ppa:jjriek/rockchip-multimedia # 添加 Rockchip 多媒体 PPA。如果不添加这个 PPA，则需要手动编译安装librga-dev
sudo apt-get update
sudo apt-get install -y build-essential gcc g++ cmake ninja-build git libopencv-dev libuvc-dev libusb-1.0-0-dev zlib1g-dev librga-dev ninja-build gdb nlohmann-json3-dev libeigen3-dev libtbb-dev
# wiringpi 需要用其他方法安装
```

对于 OrangePi 5 max，您需要安装 [wiringOP](https://github.com/orangepi-xunlong/wiringOP)。

### 第二步：克隆仓库

```bash
git clone https://github.com/yuunnn-w/orangepi_cv.git
cd orangepi_cv
```

### 第三步：根据你的相机vid和pid修改代码

在 `main.cpp` 中，找到以下代码行：

```cpp
uint16_t vid = 0x0bdc; // 厂商ID，使用lsusb命令查看
uint16_t pid = 0x0678; // 产品ID
```

将 `vid` 和 `pid` 替换为您的相机的实际厂商 ID 和产品 ID。您可以使用 `lsusb` 命令来查找这些信息。

### 第四步：根据你的舵机型号修改舵机驱动代码

在 `servo_driver.h` 中，找到以下代码行：

```cpp
#define SERVO_ANGLE_RANGE 360.0 //舵机最大转角
#define SERVO_ANGLE_HALF_RANGE 180.0 //舵机最大转角的一半
```

将 `SERVO_ANGLE_RANGE` 和 `SERVO_ANGLE_HALF_RANGE` 替换为您的舵机的实际角度范围。请根据您的舵机型号进行调整。
注意，本项目采用的是炬晖总线舵机，官方网址为 [https://johorobot.com/](https://johorobot.com/)，本项目暂不支持其他型号的舵机。

### 第五步：编译项目

创建构建目录并使用 CMake 编译项目：

```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

### 第六步：运行项目

首先，在运行项目之前，请确保您的 OrangePi 5 Max 开发板已经连接所有设备，具体设备清单如下：

- USB 摄像头
- 热成像仪
- 舵机
- 其他传感器（如有）

在确保开发板连接了局域网络后，运行以下命令启动服务器：

```bash
./orangepi_cv
```

### 第七步：访问服务器

使用连接到同一局域网下的Windows电脑，进入项目目录下的 `python` 文件夹，运行以下命令启动客户端：

```bash
python3 ui.py
```

客户端程序采用pyqt5编写，在运行该文件前，请确保您的电脑上安装了Python3和PyQt5库。您可以使用以下命令安装依赖项：

```bash
pip install numpy opencv-python pyqt5 matplotlib pynput
```

### 第八步：使用说明

1. **启动服务器**：在 OrangePi 5 Max 上运行 `./orangepi_cv` 命令启动服务器。
2. **启动客户端**：在 Windows 电脑上运行 `python3 ui.py` 命令启动客户端。
3. **连接服务器**：客户端会自动检测到局域网内在线的基站，选择想要控制的基站，点击"开始视频"即可观看实时检测画面。
4. **舵机控制**：在客户端界面上，您可以打开舵机遥控开关，然后使用键盘上的`W`、`A`、`S`、`D`键控制舵机的转动。
5. **获取检测图像**：点击"获取当前帧"按钮，服务器会将当前检测到的图像发送到客户端本地进行保存。
6. **显示实时态势图**：点击"实时态势"按钮，会打开一个态势图窗口，显示当前检测到的目标位置。
7. **系统睡眠**：点击"系统睡眠"按钮，系统会进入睡眠状态，停止检测线程。再次点击"系统唤醒"按钮，系统会恢复运行。
8. **系统关机**：点击"系统关机"按钮，系统会关闭所有线程并退出程序。（注意，此操作不可逆，请谨慎使用）

## 贡献

欢迎贡献！请阅读 [CONTRIBUTING.md](CONTRIBUTING.md) 文件，了解如何为该项目贡献代码。

## 许可证

本项目采用 GPL-3.0 许可证。有关详细信息，请参阅 [LICENSE](LICENSE) 文件。
**注意，GPL-3.0 允许您修改代码并进行二次开发，但请开源您的代码，并注明原作者和出处。**

## 联系我

如果您对本项目有任何疑问或建议，请通过以下方式与我联系：

- **维护者**: 小阳
- **电子邮箱**: [jiaxinsugar@gmail.com](mailto:jiaxinsugar@gmail.com)
- **GitHub**: [yuunnn-w](https://github.com/yuunnn-w)
