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

void controlSystemState() {
    // ����10��
    std::this_thread::sleep_for(std::chrono::seconds(10));

    // �� system_sleep ��Ϊ true
    system_sleep = true;
    std::cout << "system_sleep set to true." << std::endl;

    // ����2��
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // �� system_sleep �Ļ� false
    system_sleep = false;
    std::cout << "system_sleep set to false." << std::endl;

    // ����5��
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // �� poweroff ��Ϊ true
    poweroff = true;
    std::cout << "poweroff set to true." << std::endl;
}

int main() {
    //WebSocketServer server;
    //server.run();
    /*************************************************************************/
    // ��ʼ��USB����ͷ
    // �����������
	uint16_t vid = 0x1bcf; // ����ID��ʹ��lsusb����鿴
	uint16_t pid = 0x2cd1; // ��ƷID
    uvc_frame_format format = UVC_FRAME_FORMAT_MJPEG; // ͼ���ʽ
    int width = 1280;                                 // ͼ����
    int height = 720;                                // ͼ��߶�
    int fps = 30;                                     // ֡��
    Camera camera(vid, pid);// �����������
    std::cout << "Init Camera!" << std::endl;
    if (!camera.open()) { // ������豸
        std::cerr << "Failed to open camera device." << std::endl;
        return -1;
    }
    camera.configure_stream(format, width, height, fps); // ����������
	camera.start(); // ������
	std::cout << "Start Camera!" << std::endl;
    usleep(200000); // �ȴ�0.2�����豸������������λ΢��us��
    // ��ʼ��USB����ͷ����
    /*************************************************************************/
    //��ʼ��RKNN YOLOģ��
    std::string model_path = "/root/.vs/orangepi_cv/yolo11s.rknn";
    rknn_model model(model_path); // ��ʼ��ģ��
    int num_threads = 2; // ָ�������̵߳����������Ϊ3
    std::vector<std::thread> inference_threads; //�洢�����߳̾��
    //ģ�ͼ������
    /*************************************************************************/
    //��ʼ���������ģ��
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
    // ��ӡ���1������汾
    std::cout << "Servo Software Version: " << version << std::endl;
	servoDriver.setAngleLimits(servo1, -135.0, 125.0); // ���ö��1�ĽǶ�����
    usleep(10000); // дָ��һ��Ҫ�ȴ�10����
	servoDriver.setAngleLimits(servo2, -65, 85); // ���ö��2�ĽǶ�����
    usleep(10000);
    std::cerr << "Set angle limit done." << std::endl;
	//�������ģ���ʼ�����
    /*************************************************************************/
	// ��ʼ��ϵͳ״̬�������
    stop_inference = false;
    system_sleep = false;
    poweroff = false;
    is_sleeping = false; // ��־��������ʾϵͳ�Ƿ��Ѿ�����˯��״̬

    // ���������߳�
    start_threads(inference_threads, model, num_threads);

    // ������������߳�
    std::thread servo_thread(control_servo, std::ref(servoDriver), servo1, servo2);
    
    
    std::thread control_thread(controlSystemState); // ����ϵͳ״̬�������

    while (true) {
        if (system_sleep) {
            if (!is_sleeping) {
                // ���ϵͳδ����˯��״̬����ִ��˯�߲���
                stop_inference = true;
                join_threads(inference_threads);
                std::cerr << "System start to sleep..." << std::endl;
                is_sleeping = true; // ���ϵͳ�ѽ���˯��״̬
            }
        }
        else {
            if (is_sleeping) {
                // ���ϵͳ��˯��״̬�ָ��������������߳�
                stop_inference = false;
                start_threads(inference_threads, model, num_threads);
                std::cerr << "System restart..." << std::endl;
                is_sleeping = false; // ���ϵͳ�ѻָ�����
            }
        }
        if (poweroff) {
            std::cout << "System is powering off..." << std::endl;
            break;
        }
        usleep(100000); // ��Ϣ100����
    }

    stop_inference = true;
    join_threads(inference_threads);
    // �ȴ������߳̽���
    control_thread.join();
    camera.stop();

    return 0;
}