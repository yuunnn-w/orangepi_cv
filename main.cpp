// main.cpp
#include "server.h"
#include "camera.h"
#include "threadsafequeue.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <unistd.h>

// 循环从队列获取最新的原始图像数据
void capture_images() {
    int loop_count = 100;
    double total_time = 0.0;

    for (int i = 0; i < loop_count; ++i) {
        auto start_time = std::chrono::high_resolution_clock::now();

        cv::Mat latest = image_queue.get_latest_raw();
        if (!latest.empty()) {
            // 处理图像数据，如保存
            std::string filename = "./test.jpg";
            //std::cout << "Saved img!" << std::endl;
            cv::imwrite(filename, latest);  // 保存图像，覆盖已有文件
        }
        else
        {
            std::cout << "Empty img!" << std::endl;
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        total_time += duration;
    }

    double average_time = total_time / loop_count;
    std::cout << "Average Time:" << average_time << " ms" << std::endl;
}



int main() {
    //WebSocketServer server;
    //server.run();

    // 定义相机参数
    uint16_t vid = 0x1bcf;  // 替换为实际的 Vendor ID
    uint16_t pid = 0x2cd1;  // 替换为实际的 Product ID
    uvc_frame_format format = UVC_FRAME_FORMAT_MJPEG;  // 图像格式
    int width = 1920;                                // 图像宽度
    int height = 1080;                               // 图像高度
    int fps = 60;                                   // 帧率

    Camera camera(vid, pid);// 创建相机对象
    std::cout << "Init camera!" << std::endl;
    if (!camera.open()) { // 打开相机设备
        std::cerr << "Failed to open camera device." << std::endl;
        return -1;
    }
    if (!camera.configure_stream(format, width, height, fps)) { // 配置流参数
        std::cerr << "Failed to configure stream parameters." << std::endl;
        return -1;
    }
    if (!camera.start()) { // 启动流
        std::cerr << "Failed to start streaming." << std::endl;
        return -1;
    }
    std::cout << "Start Catching!" << std::endl;
    usleep(200000); // 200000 microseconds = 0.2 seconds 等待0.2秒让设备彻底启动（微秒us为单位）
    // 启动一个线程来捕获图像
    std::thread capture_thread(capture_images);
    capture_thread.join();
    camera.stop();
    return 0;
}
