// threadsafequeue.cpp
#include "threadsafequeue.h"

ThreadSafeQueue image_queue;

void ThreadSafeQueue::push_raw(const cv::Mat& img) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (img.empty()) {
        std::cerr << "Warning: Attempted to push an empty image in push_raw()" << std::endl;
        return;
    }

    // 生成序号
    uint64_t seq_num = sequence_number_.fetch_add(1, std::memory_order_relaxed);

    // 如果队列超过最大长度，删除最早的元素
    if (raw_queue.size() >= max_raw_size_) {
        raw_queue.pop_front();
    }
    // std::cerr << "New Frame: " << seq_num << std::endl;
    // 将新图像和序号入队
    raw_queue.push_back({ img.clone(), seq_num });
}

void ThreadSafeQueue::push_inference(uint64_t sequence_number, const cv::Mat& img, const std::string& json_result, const object_detect_result_list& result_list) {
    std::unique_lock<std::mutex> lock(mutex_);

    // 将推理结果按序号插入到正确的位置
    inference_queue[sequence_number] = { img, json_result, result_list };

    // 如果队列超过最大长度，删除最旧的元素（序号最小的元素）
    if (inference_queue.size() > max_inference_size_) {
        inference_queue.erase(inference_queue.begin());
    }

    // 更新帧率统计
    update_fps();
}

std::pair<uint64_t, cv::Mat> ThreadSafeQueue::get_latest_raw() {
    std::unique_lock<std::mutex> lock(mutex_);

    if (!raw_queue.empty()) {
        auto latest = raw_queue.back();  // 获取最新元素

        // 在锁的保护下完成深拷贝
        uint64_t seq_num = latest.second;
        cv::Mat img = latest.first;  // 深拷贝图像

        raw_queue.pop_back();  // 删除最新元素
        lock.unlock();         // 释放锁

        // 返回深拷贝的结果
        return { seq_num, img };
    }

    return { 0, cv::Mat() };  // 如果队列为空，返回空的 cv::Mat 和序号 0
}

std::tuple<uint64_t, cv::Mat, std::string, object_detect_result_list> ThreadSafeQueue::get_latest_inference() {
    std::unique_lock<std::mutex> lock(mutex_);

    if (!inference_queue.empty()) {
        auto latest = inference_queue.rbegin();  // 获取最新元素（序号最大的元素）

        // 在锁的保护下完成深拷贝
        uint64_t seq_num = latest->first;
        cv::Mat img = std::get<0>(latest->second).clone();  // 深拷贝图像
        std::string json_result = std::get<1>(latest->second);  // 拷贝 JSON 字符串
        object_detect_result_list result_list = std::get<2>(latest->second);  // 拷贝推理结果结构体

        lock.unlock();  // 完成深拷贝后释放锁

        // 返回深拷贝的结果
        return { seq_num, img, json_result, result_list };
    }

    return { 0, cv::Mat(), "", {} };  // 如果队列为空，返回空值
}