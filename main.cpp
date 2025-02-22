// main.cpp

#include "server.h"
#include "camera.h"
#include "threadsafequeue.h"
#include "rknn_model.h"
#include "utils.h"
#include "servo_driver.h"

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

void controlSystemState() {
    // 休眠10秒
    std::this_thread::sleep_for(std::chrono::seconds(10));

    // 将 system_sleep 改为 true
    system_sleep = true;
    std::cout << "system_sleep set to true." << std::endl;

    // 休眠2秒
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // 将 system_sleep 改回 false
    system_sleep = false;
    std::cout << "system_sleep set to false." << std::endl;

    // 休眠5秒
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // 将 poweroff 改为 true
    poweroff = true;
    std::cout << "poweroff set to true." << std::endl;
}

int main() {
    //WebSocketServer server;
    //server.run();
    /*************************************************************************/
    // 初始化USB摄像头
    // 定义相机参数
	uint16_t vid = 0x1bcf; // 厂商ID，使用lsusb命令查看
	uint16_t pid = 0x2cd1; // 产品ID
    uvc_frame_format format = UVC_FRAME_FORMAT_MJPEG; // 图像格式
    int width = 1280;                                 // 图像宽度
    int height = 720;                                // 图像高度
    int fps = 30;                                     // 帧率
    Camera camera(vid, pid);// 创建相机对象
    std::cout << "Init Camera!" << std::endl;
    if (!camera.open()) { // 打开相机设备
        std::cerr << "Failed to open camera device." << std::endl;
        return -1;
    }
    camera.configure_stream(format, width, height, fps); // 配置流参数
	camera.start(); // 启动流
	std::cout << "Start Camera!" << std::endl;
    usleep(200000); // 等待0.2秒让设备彻底启动（单位微秒us）
    // 初始化USB摄像头结束
    /*************************************************************************/
    //初始化RKNN YOLO模型
    std::string model_path = "/root/.vs/orangepi_cv/yolo11s.rknn";
    rknn_model model(model_path); // 初始化模型
    int num_threads = 2; // 指定推理线程的数量，最大为3
    std::vector<std::thread> inference_threads; //存储推理线程句柄
    //模型加载完毕
    /*************************************************************************/
    //初始化舵机控制模块
    ServoDriver servoDriver("/dev/ttyUSB0");
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
	servoDriver.setAngleLimits(servo1, -135.0, 125.0); // 设置舵机1的角度限制
    usleep(10000); // 写指令一般要等待10毫秒
	servoDriver.setAngleLimits(servo2, -65, 85); // 设置舵机2的角度限制
    usleep(10000);
    std::cerr << "Set angle limit done." << std::endl;
	//舵机控制模块初始化完毕
    /*************************************************************************/
	// 初始化系统状态管理变量
    stop_inference = false;
    system_sleep = false;
    poweroff = false;
    is_sleeping = false; // 标志变量，表示系统是否已经进入睡眠状态

    // 启动推理线程
    start_threads(inference_threads, model, num_threads);

    // 启动舵机控制线程
    std::thread servo_thread(control_servo, std::ref(servoDriver), servo1, servo2);
    
    
    std::thread control_thread(controlSystemState); // 启动系统状态管理测试

    while (true) {
        if (system_sleep) {
            if (!is_sleeping) {
                // 如果系统未进入睡眠状态，则执行睡眠操作
                stop_inference = true;
                join_threads(inference_threads);
                std::cerr << "System start to sleep..." << std::endl;
                is_sleeping = true; // 标记系统已进入睡眠状态
            }
        }
        else {
            if (is_sleeping) {
                // 如果系统从睡眠状态恢复，则重新启动线程
                stop_inference = false;
                start_threads(inference_threads, model, num_threads);
                std::cerr << "System restart..." << std::endl;
                is_sleeping = false; // 标记系统已恢复运行
            }
        }
        if (poweroff) {
            std::cout << "System is powering off..." << std::endl;
            break;
        }
        usleep(100000); // 休息100毫秒
    }

    stop_inference = true;
    join_threads(inference_threads);
    // 等待控制线程结束
    control_thread.join();
    camera.stop();

    return 0;
}