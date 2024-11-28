// main.cpp
#include "server.h"
#include "camera.h"
#include "threadsafequeue.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <unistd.h>

// ѭ���Ӷ��л�ȡ���µ�ԭʼͼ������
void capture_images() {
    int loop_count = 100;
    double total_time = 0.0;

    for (int i = 0; i < loop_count; ++i) {
        auto start_time = std::chrono::high_resolution_clock::now();

        cv::Mat latest = image_queue.get_latest_raw();
        if (!latest.empty()) {
            // ����ͼ�����ݣ��籣��
            std::string filename = "/root/code/test.jpg";
            //std::cout << "Saved img!" << std::endl;
            cv::imwrite(filename, latest);  // ����ͼ�񣬸��������ļ�
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

    // �����������
    uint16_t vid = 0x1bcf;  // �滻Ϊʵ�ʵ� Vendor ID
    uint16_t pid = 0x2cd1;  // �滻Ϊʵ�ʵ� Product ID
    uvc_frame_format format = UVC_FRAME_FORMAT_MJPEG;  // ͼ���ʽ
    int width = 1920;                                // ͼ����
    int height = 1080;                               // ͼ��߶�
    int fps = 60;                                   // ֡��

    Camera camera(vid, pid);// �����������
    std::cout << "Init camera!" << std::endl;
    if (!camera.open()) { // ������豸
        std::cerr << "Failed to open camera device." << std::endl;
        return -1;
    }
    if (!camera.configure_stream(format, width, height, fps)) { // ����������
        std::cerr << "Failed to configure stream parameters." << std::endl;
        return -1;
    }
    if (!camera.start()) { // ������
        std::cerr << "Failed to start streaming." << std::endl;
        return -1;
    }
    std::cout << "Start Catching!" << std::endl;
    usleep(200000); // 200000 microseconds = 0.2 seconds �ȴ�0.2�����豸����������΢��usΪ��λ��
    // ����һ���߳�������ͼ��
    std::thread capture_thread(capture_images);
    capture_thread.join();
    camera.stop();
    return 0;
}