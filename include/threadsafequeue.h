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

    // 入队原始图像数据，并生成序号
    void push_raw(const cv::Mat& img);

    // 入队推理结果，支持 JSON 格式和 object_detect_result_list 结构体
    void push_inference(uint64_t sequence_number, const cv::Mat& img, const std::string& json_result, const object_detect_result_list& result_list, const std::vector<float>& position);

    // 获取队列中的最新原始图像数据（深拷贝）及其序号
    std::tuple<uint64_t, cv::Mat, std::vector<float>> get_latest_raw();

    // 获取推理结果中的最新元素（深拷贝）及其序号
    std::tuple<uint64_t, cv::Mat, std::string, object_detect_result_list, std::vector<float>> get_latest_inference();

private:
    // std::deque<std::pair<cv::Mat, uint64_t>> raw_queue;  // 原始图像队列，存储图像和序号
    std::deque<std::tuple<cv::Mat, uint64_t, std::vector<float>>> raw_queue; // 包括舵机位置
    std::map<uint64_t, std::tuple<cv::Mat, std::string, object_detect_result_list, std::vector<float>>> inference_queue; // 推理结果队列，按序号排序
    std::mutex mutex_;  // 保护队列操作的互斥锁
    size_t max_raw_size_;  // 原始图像队列的最大长度
    size_t max_inference_size_;  // 推理结果队列的最大长度
    std::atomic<uint64_t> sequence_number_;  // 全局序号生成器

    // 帧率统计相关变量
    int frame_count = 0; // 帧数计数器
    std::chrono::high_resolution_clock::time_point last_time = std::chrono::high_resolution_clock::now(); // 上一次统计时间

    void update_fps() {
        auto now = std::chrono::high_resolution_clock::now(); // 当前时间
        frame_count++; // 增加帧数

        // 计算时间间隔
        auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_time).count() / 1000.0; // 转换为秒

        if (elapsed_time >= 1.0) { // 每1秒计算一次帧率
            double fps = frame_count / elapsed_time; // 计算帧率
            std::cout << "FPS: " << fps << std::endl; // 打印帧率

            // 重置帧数和时间
            frame_count = 0;
            last_time = now;
        }
    }


};

extern ThreadSafeQueue image_queue;

#endif // THREAD_SAFE_QUEUE_H
