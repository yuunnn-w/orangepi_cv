// threadsafequeue.cpp
#include "threadsafequeue.h"

ThreadSafeQueue image_queue;

void ThreadSafeQueue::push_raw(const cv::Mat& img) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (img.empty()) {
        std::cerr << "Warning: Attempted to push an empty image in push_raw()" << std::endl;
        return;
    }
    if (raw_queue.size() >= 2) {
        raw_queue.pop();  // 如果队列超过最大长度，删除最早的元素
    }
    raw_queue.push(img);  // 将新图像入队
}

void ThreadSafeQueue::push_inference(const cv::Mat& img, const std::string& result) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (inference_queue.size() >= 2) {
        inference_queue.pop();  // 如果推理结果队列超过最大长度，删除最早的元素
    }
    inference_queue.push({ img, result });  // 入队推理结果
}

cv::Mat ThreadSafeQueue::get_latest_raw() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!raw_queue.empty()) {
        // cv::Mat latest = raw_queue.back().clone();  // 深拷贝队列中的最新图像
        return raw_queue.back().clone();  // 返回深拷贝队列中的最新图像
    }
    return cv::Mat();  // 如果队列为空，返回空的 cv::Mat
}

std::pair<cv::Mat, std::string> ThreadSafeQueue::get_latest_inference() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!inference_queue.empty()) {
        // cv::Mat latest_img = inference_queue.back().first.clone();
        // std::string latest_result = inference_queue.back().second;
        return { inference_queue.back().first.clone(), inference_queue.back().second };  // 返回最新的推理结果（深拷贝）
    }
    return { cv::Mat(), "" };  // 如果队列为空，返回空的 cv::Mat 和空的推理结果
}
