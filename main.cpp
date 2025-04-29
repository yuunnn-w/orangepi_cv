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

// �����̵߳� CPU �׺���
void setThreadAffinity(std::thread& thread, int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    int result = pthread_setaffinity_np(thread.native_handle(), sizeof(cpu_set_t), &cpuset);
    if (result != 0) {
        std::cerr << "Failed to set thread affinity: " << result << std::endl;
    }
}

// �����̵߳ĺ���
void start_threads(std::vector<std::thread>& inference_threads, rknn_model& model, int num_threads) {
    for (int i = 0; i < num_threads; ++i) {
        inference_threads.emplace_back(inference_images, std::ref(model), i); // ���������߳�
        setThreadAffinity(inference_threads.back(), 4 + i); // �󶨵�����4,5,6
    }
}

// �ȴ��������̵߳ĺ���
void join_threads(std::vector<std::thread>& inference_threads) {
    for (size_t i = 0; i < inference_threads.size(); ++i) {
        if (inference_threads[i].joinable()) {
            inference_threads[i].join();
        }
    }
    inference_threads.clear();
}

int main() {
    // �򿪴���
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
    // ��ӡ���1������汾
    std::cout << "Servo Software Version: " << version << std::endl;
    // �������λ��0��
    servoDriver.setTargetPosition(servo1, 0);
    usleep(10000);
    servoDriver.setTargetPosition(servo2, 45);
    usleep(10000);
    std::cerr << "Servo reset done." << std::endl;
    //�������ģ���ʼ�����

    // ��ʼ��USB����ͷ
    // �����������
    // 1bcf:2cd1
    // 0bdc:0678
	uint16_t vid = 0x0bdc; // ����ID��ʹ��lsusb����鿴
	uint16_t pid = 0x0678; // ��ƷID
    uvc_frame_format format = UVC_FRAME_FORMAT_MJPEG; // ͼ���ʽ
    int width = 1280;                                 // ͼ����
    int height = 720;                                 // ͼ��߶�
    int fps = 30;                                     // ֡��
    Camera camera(vid, pid);// �����������
    std::cout << "Init Camera!" << std::endl;
    if (!camera.open()) { // ������豸
        std::cerr << "Failed to open camera device." << std::endl;
        return -1;
    }
    camera.configure_stream(format, width, height, fps); // ����������
    // ��ʼ��USB����ͷ����
    
    // ��ʼ���ȳ�����
    IRSensor sensor(0x2bdf, 0x0102); // Hikmicro's VID/PID
    if (!sensor.initDevice()) {
        std::cerr << "Device initialization failed" << std::endl;
        return 1;
    }
    IRSensor::DeviceInfo info;
    if (sensor.getDeviceInfo(info)) {
        std::cout << "=== Device Information ===" << std::endl;
        std::cout << "Firmware Version: " << info.firmwareVersion << std::endl;
        // ˯��100����
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
    // �����豸ʱ��
    if (!sensor.configureDateTime()) {
        std::cerr << "\nFailed to configure device time" << std::endl;
    }
    // ������������
    if (!sensor.configureStreamType(6)) {
        std::cerr << "\nFailed to configure stream type" << std::endl;
    }
    // ����ͼ����ǿ
    IRSensor::ImageEnhancementParams imgParams;
    if (!sensor.configureImageEnhancement(imgParams)) {
        std::cerr << "Image enhancement configuration failed" << std::endl;
    }
    // ������Ƶ����
    IRSensor::VideoAdjustmentParams videoParams;
    if (!sensor.configureVideoAdjustment(videoParams)) {
        std::cerr << "Video adjustment configuration failed" << std::endl;
    }

    // ������
    IRSensor::StreamConfig streamConfig;
    streamConfig.width = 256;
    streamConfig.height = 192;
    streamConfig.fps = 25;
    streamConfig.format = UVC_FRAME_FORMAT_YUYV;
    // streamConfig.format = UVC_FRAME_FORMAT_MJPEG;
    if (!sensor.configureStream(streamConfig)) {
        std::cerr << "Stream configuration failed" << std::endl;
    }
    // ������
    if (!sensor.startStreaming()) {
        std::cerr << "Failed to start IRSensor." << std::endl;
        return -1;
    }
	if (!camera.start()) { // �������
		std::cerr << "Failed to start camera." << std::endl;
        return -1;
	}
    std::cout << "Start Camera Done!" << std::endl;
    usleep(300000); // �ȴ�0.3�����豸������������λ΢��us��
	// �ȳ����ǳ�ʼ�����
    
    //��ʼ��RKNN YOLOģ��
    std::string model_path = "/root/.vs/orangepi_cv/yolo11s.rknn";
    rknn_model model(model_path); // ��ʼ��ģ��
    int num_threads = 3; // ָ�������̵߳����������Ϊ3
    std::vector<std::thread> inference_threads; //�洢�����߳̾��
    //ģ�ͼ������
    
	// ��ʼ��ϵͳ״̬�������
    stop_inference = false;
    system_sleep = false;
    poweroff = false;
    is_sleeping = false; // ��־��������ʾϵͳ�Ƿ��Ѿ�����˯��״̬

    // ���������߳�
    start_threads(inference_threads, model, num_threads); // �������������߳�
	std::cerr << "Inference threads started." << std::endl;
    usleep(200000);
    // ������������߳�
    std::thread servo_thread(control_servo);
	std::cerr << "Servo control thread started." << std::endl;
	usleep(200000); // ��Ϣ200����
	// ����������
    TCPServer server;
	server.run(8080);
	std::cerr << "Server started." << std::endl;

    // ����״̬����
    static int dark_counter = 0;
    static int light_counter = 0;

    while (true) {
        if (system_sleep) { //���Ҫ��ϵͳ����˯��״̬
            if (!is_sleeping) {
                // ���ϵͳδ����˯��״̬����ִ��˯�߲���
                stop_inference = true;
                join_threads(inference_threads); // �ȴ������߳̽���
                servo_thread.join(); // �ȴ���������߳̽���
                std::cerr << "System start to sleep..." << std::endl;
                is_sleeping = true; // ���ϵͳ�ѽ���˯��״̬
            }
        }
        else {
            if (is_sleeping) {
                // ���ϵͳ��˯��״̬�ָ��������������߳�
                stop_inference = false;
                start_threads(inference_threads, model, num_threads); // �������������߳�
                servo_thread = std::thread(control_servo); // ����������������߳�
                // std::thread servo_thread(control_servo);
                std::cerr << "System restart..." << std::endl;
                is_sleeping = false; // ���ϵͳ�ѻָ�����
            }
        }
        if (poweroff) { // ���Ҫ��ϵͳ�ػ�
            std::cout << "System is powering off..." << std::endl;
            break;
        }
        // �����ȡ�ɼ���ͼ���ж��Ƿ����̫�������̫�����л����ģʽ
        cv::Mat visible_frame = camera.get_frame();
        // std::cerr << "get frame...." << std::endl;
        if (visible_frame.empty()) {
            std::cerr << "Failed to capture frame." << std::endl;
            continue;
        }
        else {
            // ת��Ϊ�Ҷ�ͼ��������
            cv::Mat gray_frame;
            cv::cvtColor(visible_frame, gray_frame, cv::COLOR_RGB2GRAY);
            double avg_brightness = cv::mean(gray_frame)[0];
            // printf("Average brightness: %.2f\n", avg_brightness);
            // std::cerr << "Average brightness: " << avg_brightness << std::endl;
            if (!camera_mode.load()) {
                // ��ǰ�ǿɼ���ģʽ������Ƿ���Ҫ�к���
                if (avg_brightness < DARK_THRESHOLD) {
                    dark_counter++;
                    light_counter = 0; // ������һ״̬������
                    if (dark_counter >= SWITCH_FRAMES) {
                        camera_mode.store(true);
                        dark_counter = 0; // �л�������
                    }
                }
                else {
                    dark_counter = 0; // δ���������
                }
            }
            else {
                // ��ǰ�Ǻ���ģʽ������Ƿ���Ҫ�пɼ���
                if (avg_brightness > LIGHT_THRESHOLD) {
                    light_counter++;
                    dark_counter = 0; // ������һ״̬������
                    if (light_counter >= SWITCH_FRAMES) {
                        camera_mode.store(false);
                        light_counter = 0; // �л�������
                    }
                }
                else {
                    light_counter = 0; // δ���������
                }
            }
        }
        usleep(100000); // ��Ϣ100����
    }
	std::cout << "System is shutting down..." << std::endl;
    stop_inference = true;
	join_threads(inference_threads); // �ȴ������߳̽���
	servo_thread.join(); // �ȴ���������߳̽���
	camera.stop(); // ֹͣ�������ͼ��
	sensor.stopStreaming(); // �ر��ȳ�����
	server.stop(); // ֹͣ������

    return 0; 
}

// **ע��**������Ŀ����ѧϰ���о�ʹ�ã�����������ҵ��;�������κ���Ȩ��Ϊ������ϵ��ɾ��������ݡ�


















*/

