// threadsafequeue.h
#ifndef THREAD_SAFE_QUEUE_H
#define THREAD_SAFE_QUEUE_H

#include <queue>
#include <map>
#include <mutex>
#include <atomic>
#include <string>
#include <iostream>
#include <opencv2/opencv.hpp>
#include "common.h"
#include "servo_driver.h"
#include "utils.h"

class ThreadSafeQueue {
public:
    ThreadSafeQueue(size_t max_raw_size = 2, size_t max_inference_size = 10)
        : max_raw_size_(max_raw_size), max_inference_size_(max_inference_size), sequence_number_(0) {
    }

    // ���ԭʼͼ�����ݣ����������
    void push_raw(const cv::Mat& img);

    // �����������֧�� JSON ��ʽ�� object_detect_result_list �ṹ��
    void push_inference(uint64_t sequence_number, const cv::Mat& img, const std::string& json_result, const object_detect_result_list& result_list, const std::vector<float>& position);

    // ��ȡ�����е�����ԭʼͼ�����ݣ�������������
    std::tuple<uint64_t, cv::Mat, std::vector<float>> get_latest_raw();

    // ��ȡ�������е�����Ԫ�أ�������������
    std::tuple<uint64_t, cv::Mat, std::string, object_detect_result_list, std::vector<float>> get_latest_inference();

private:
    // std::deque<std::pair<cv::Mat, uint64_t>> raw_queue;  // ԭʼͼ����У��洢ͼ������
    std::deque<std::tuple<cv::Mat, uint64_t, std::vector<float>>> raw_queue; // �������λ��
    std::map<uint64_t, std::tuple<cv::Mat, std::string, object_detect_result_list, std::vector<float>>> inference_queue; // ���������У����������
    std::mutex mutex_;  // �������в����Ļ�����
    size_t max_raw_size_;  // ԭʼͼ����е���󳤶�
    size_t max_inference_size_;  // ���������е���󳤶�
    std::atomic<uint64_t> sequence_number_;  // ȫ�����������

    // ֡��ͳ����ر���
    int frame_count = 0; // ֡��������
    std::chrono::high_resolution_clock::time_point last_time = std::chrono::high_resolution_clock::now(); // ��һ��ͳ��ʱ��

    void update_fps() {
        auto now = std::chrono::high_resolution_clock::now(); // ��ǰʱ��
        frame_count++; // ����֡��

        // ����ʱ����
        auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_time).count() / 1000.0; // ת��Ϊ��

        if (elapsed_time >= 1.0) { // ÿ1�����һ��֡��
            double fps = frame_count / elapsed_time; // ����֡��
            std::cout << "FPS: " << fps << std::endl; // ��ӡ֡��

            // ����֡����ʱ��
            frame_count = 0;
            last_time = now;
        }
    }


};

extern ThreadSafeQueue image_queue;

#endif // THREAD_SAFE_QUEUE_H
