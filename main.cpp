// main.cpp
#include "camera.h"
#include "threadsafequeue.h"
#include "rknn_model.h"
#include "utils.h"
#include "servo_driver.h"
#include "server.h"
#include "IRSensor.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <unistd.h>

#include <pthread.h>
#include <sched.h>

// 设置线程的 CPU 亲和性
void setThreadAffinity(std::thread& thread, int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    int result = pthread_setaffinity_np(thread.native_handle(), sizeof(cpu_set_t), &cpuset);
    if (result != 0) {
        std::cerr << "Failed to set thread affinity: " << result << std::endl;
    }
}

// 启动线程的函数
void start_threads(std::vector<std::thread>& inference_threads, rknn_model& model, int num_threads) {
    for (int i = 0; i < num_threads; ++i) {
        inference_threads.emplace_back(inference_images, std::ref(model), i); // 启动推理线程
        setThreadAffinity(inference_threads.back(), 4 + i); // 绑定到核心4,5,6
    }
}

// 等待并清理线程的函数
void join_threads(std::vector<std::thread>& inference_threads) {
    for (size_t i = 0; i < inference_threads.size(); ++i) {
        if (inference_threads[i].joinable()) {
            inference_threads[i].join();
        }
    }
    inference_threads.clear();
}

int main() {
    // 打开串口
    if (servoDriver.openPort()) {
        std::cerr << "Open port successfully." << std::endl;
    }
    else {
        std::cerr << "Failed to open port." << std::endl;
        return -1;
    }
    uint8_t servo1 = 1;
    uint8_t servo2 = 2;
    std::string version = servoDriver.getSoftwareVersion(servo1);
    // 打印舵机1的软件版本
    std::cout << "Servo Software Version: " << version << std::endl;
    // 将舵机复位至0度
    servoDriver.setTargetPosition(servo1, 0);
    usleep(10000);
    servoDriver.setTargetPosition(servo2, 45);
    usleep(10000);
    std::cerr << "Servo reset done." << std::endl;
    //舵机控制模块初始化完毕

    // 初始化USB摄像头
    // 定义相机参数
    // 1bcf:2cd1
    // 0bdc:0678
	uint16_t vid = 0x0bdc; // 厂商ID，使用lsusb命令查看
	uint16_t pid = 0x0678; // 产品ID
    uvc_frame_format format = UVC_FRAME_FORMAT_MJPEG; // 图像格式
    int width = 1280;                                 // 图像宽度
    int height = 720;                                 // 图像高度
    int fps = 30;                                     // 帧率
    Camera camera(vid, pid);// 创建相机对象
    std::cout << "Init Camera!" << std::endl;
    if (!camera.open()) { // 打开相机设备
        std::cerr << "Failed to open camera device." << std::endl;
        return -1;
    }
    camera.configure_stream(format, width, height, fps); // 配置流参数
    // 初始化USB摄像头结束
    
    // 初始化热成像仪
    IRSensor sensor(0x2bdf, 0x0102); // Hikmicro's VID/PID
    if (!sensor.initDevice()) {
        std::cerr << "Device initialization failed" << std::endl;
        return 1;
    }
    IRSensor::DeviceInfo info;
    if (sensor.getDeviceInfo(info)) {
        std::cout << "=== Device Information ===" << std::endl;
        std::cout << "Firmware Version: " << info.firmwareVersion << std::endl;
        // 睡眠100毫秒
        usleep(100000);
        std::cout << "\nEncoder Version: " << info.encoderVersion << std::endl;
        usleep(100000);
        std::cout << "\nHardware Version: " << info.hardwareVersion << std::endl;
        usleep(100000);
        std::cout << "\nDevice Name: " << info.deviceName << std::endl;
        usleep(100000);
        std::cout << "\nProtocol Version: " << info.protocolVersion << std::endl;
        usleep(100000);
        std::cout << "\nSerial Number: " << info.serialNumber << std::endl;
        usleep(100000);
    }
	else {
		std::cerr << "Failed to get device information" << std::endl;
		return 1;
	}
    // 配置设备时间
    if (!sensor.configureDateTime()) {
        std::cerr << "\nFailed to configure device time" << std::endl;
    }
    // 配置码流类型
    if (!sensor.configureStreamType(6)) {
        std::cerr << "\nFailed to configure stream type" << std::endl;
    }
    // 配置图像增强
    IRSensor::ImageEnhancementParams imgParams;
    if (!sensor.configureImageEnhancement(imgParams)) {
        std::cerr << "Image enhancement configuration failed" << std::endl;
    }
    // 配置视频调整
    IRSensor::VideoAdjustmentParams videoParams;
    if (!sensor.configureVideoAdjustment(videoParams)) {
        std::cerr << "Video adjustment configuration failed" << std::endl;
    }

    // 配置流
    IRSensor::StreamConfig streamConfig;
    streamConfig.width = 256;
    streamConfig.height = 192;
    streamConfig.fps = 25;
    streamConfig.format = UVC_FRAME_FORMAT_YUYV;
    // streamConfig.format = UVC_FRAME_FORMAT_MJPEG;
    if (!sensor.configureStream(streamConfig)) {
        std::cerr << "Stream configuration failed" << std::endl;
    }
    // 启动流
    if (!sensor.startStreaming()) {
        std::cerr << "Failed to start IRSensor." << std::endl;
        return -1;
    }
	if (!camera.start()) { // 启动相机
		std::cerr << "Failed to start camera." << std::endl;
        return -1;
	}
    std::cout << "Start Camera Done!" << std::endl;
    usleep(300000); // 等待0.3秒让设备彻底启动（单位微秒us）
	// 热成像仪初始化完毕
    
    //初始化RKNN YOLO模型
    std::string model_path = "/root/.vs/orangepi_cv/yolo11s.rknn";
    rknn_model model(model_path); // 初始化模型
    int num_threads = 3; // 指定推理线程的数量，最大为3
    std::vector<std::thread> inference_threads; //存储推理线程句柄
    //模型加载完毕
    
	// 初始化系统状态管理变量
    stop_inference = false;
    system_sleep = false;
    poweroff = false;
    is_sleeping = false; // 标志变量，表示系统是否已经进入睡眠状态

    // 启动推理线程
    start_threads(inference_threads, model, num_threads); // 重新启动推理线程
	std::cerr << "Inference threads started." << std::endl;
    usleep(200000);
    // 启动舵机控制线程
    std::thread servo_thread(control_servo);
	std::cerr << "Servo control thread started." << std::endl;
	usleep(200000); // 休息200毫秒
	// 启动服务器
    TCPServer server;
	server.run(8080);
	std::cerr << "Server started." << std::endl;

    // 亮暗状态计数
    static int dark_counter = 0;
    static int light_counter = 0;

    while (true) {
        if (system_sleep) { //如果要求系统进入睡眠状态
            if (!is_sleeping) {
                // 如果系统未进入睡眠状态，则执行睡眠操作
                stop_inference = true;
                join_threads(inference_threads); // 等待推理线程结束
                servo_thread.join(); // 等待舵机控制线程结束
                std::cerr << "System start to sleep..." << std::endl;
                is_sleeping = true; // 标记系统已进入睡眠状态
            }
        }
        else {
            if (is_sleeping) {
                // 如果系统从睡眠状态恢复，则重新启动线程
                stop_inference = false;
                start_threads(inference_threads, model, num_threads); // 重新启动推理线程
                servo_thread = std::thread(control_servo); // 重新启动舵机控制线程
                // std::thread servo_thread(control_servo);
                std::cerr << "System restart..." << std::endl;
                is_sleeping = false; // 标记系统已恢复运行
            }
        }
        if (poweroff) { // 如果要求系统关机
            std::cout << "System is powering off..." << std::endl;
            break;
        }
        // 这里获取可见光图像并判断是否光线太暗，如果太暗就切换相机模式
        cv::Mat visible_frame = camera.get_frame();
        // std::cerr << "get frame...." << std::endl;
        if (visible_frame.empty()) {
            std::cerr << "Failed to capture frame." << std::endl;
            continue;
        }
        else {
            // 转换为灰度图计算亮度
            cv::Mat gray_frame;
            cv::cvtColor(visible_frame, gray_frame, cv::COLOR_RGB2GRAY);
            double avg_brightness = cv::mean(gray_frame)[0];
            // printf("Average brightness: %.2f\n", avg_brightness);
            // std::cerr << "Average brightness: " << avg_brightness << std::endl;
            if (!camera_mode.load()) {
                // 当前是可见光模式：检测是否需要切红外
                if (avg_brightness < DARK_THRESHOLD) {
                    dark_counter++;
                    light_counter = 0; // 重置另一状态计数器
                    if (dark_counter >= SWITCH_FRAMES) {
                        camera_mode.store(true);
                        dark_counter = 0; // 切换后重置
                    }
                }
                else {
                    dark_counter = 0; // 未达标则重置
                }
            }
            else {
                // 当前是红外模式：检测是否需要切可见光
                if (avg_brightness > LIGHT_THRESHOLD) {
                    light_counter++;
                    dark_counter = 0; // 重置另一状态计数器
                    if (light_counter >= SWITCH_FRAMES) {
                        camera_mode.store(false);
                        light_counter = 0; // 切换后重置
                    }
                }
                else {
                    light_counter = 0; // 未达标则重置
                }
            }
        }
        usleep(100000); // 休息100毫秒
    }
	std::cout << "System is shutting down..." << std::endl;
    stop_inference = true;
	join_threads(inference_threads); // 等待推理线程结束
	servo_thread.join(); // 等待舵机控制线程结束
	camera.stop(); // 停止相机捕获图像
	sensor.stopStreaming(); // 关闭热成像仪
	server.stop(); // 停止服务器

    return 0; 
}

// **注意**：本项目仅供学习和研究使用，请勿用于商业用途。若有任何侵权行为，请联系我删除相关内容。


















*/

