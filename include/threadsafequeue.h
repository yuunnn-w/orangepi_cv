// threadsafequeue.h
#ifndef THREAD_SAFE_QUEUE_H
#define THREAD_SAFE_QUEUE_H

#include <opencv2/opencv.hpp>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <utility>

class ThreadSafeQueue {
    public:
        // 入队原始图像数据
        void push_raw(const cv::Mat& img);

        // 入队推理结果
        void push_inference(const cv::Mat& img, const std::string& result);

        // 获取队列中的最新原始图像数据（深拷贝）
        cv::Mat get_latest_raw();

        // 获取推理结果中的最新元素（深拷贝）
        std::pair<cv::Mat, std::string> get_latest_inference();

    private:
        std::queue<cv::Mat> raw_queue;
        std::queue<std::pair<cv::Mat, std::string>> inference_queue;
        std::mutex mutex_;  // 保护队列操作的互斥锁
};

extern ThreadSafeQueue image_queue; // 全局变量

#endif // THREAD_SAFE_QUEUE_H
