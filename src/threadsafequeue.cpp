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
	// 先获取当前序号，然后加1，返回原来的序号

    // 如果队列超过最大长度，删除最早的元素
    if (raw_queue.size() >= max_raw_size_) {
        raw_queue.pop_front();
    }
    // std::cerr << "New Frame: " << seq_num << std::endl;
    // 获取当前舵机位置
    // 舵机互斥锁保护
	std::unique_lock<std::mutex> servo_lock(servo_mtx);
    float x_angle = servoDriver.getCurrentPosition(1);
    float y_angle = servoDriver.getCurrentPosition(2);
	servo_lock.unlock();
	// std::cout << "Servo1 Angle: " << x_angle << std::endl; //水平转动舵机
	// std::cout << "Servo2 Angle: " << y_angle << std::endl; //垂直转动舵机
    // 构建舵机位置的vector
	std::vector<float> servo_position = { x_angle, y_angle };
    // 将新图像、序号和舵机位置入队
    raw_queue.emplace_back(img.clone(), seq_num, servo_position);
}

void ThreadSafeQueue::push_inference(uint64_t sequence_number, const cv::Mat& img, const std::string& json_result, const object_detect_result_list& result_list, const std::vector<float>& position) {
    std::unique_lock<std::mutex> lock(mutex_);

    // 将推理结果按序号插入到正确的位置
    inference_queue[sequence_number] = { img, json_result, result_list, position };

    // 如果队列超过最大长度，删除最旧的元素（序号最小的元素）
    if (inference_queue.size() > max_inference_size_) {
        inference_queue.erase(inference_queue.begin());
    }

    // 更新帧率统计
    update_fps();
}

std::tuple<uint64_t, cv::Mat, std::vector<float>> ThreadSafeQueue::get_latest_raw() {
    std::unique_lock<std::mutex> lock(mutex_);

    if (!raw_queue.empty()) {
        auto latest = raw_queue.back();  // 获取最新元素

        // 在锁的保护下完成深拷贝
        uint64_t seq_num = std::get<uint64_t>(latest);
		cv::Mat img = std::get<cv::Mat>(latest).clone();  // 深拷贝图像
		std::vector<float> position = std::get<std::vector<float>>(latest);  // 深拷贝舵机位置
        raw_queue.pop_back();  // 删除最新元素
        lock.unlock();         // 释放锁

        // 返回深拷贝的结果
        return { seq_num, img, position };
    }

	return { 0, cv::Mat(), {} };  // 如果队列为空，返回空值
}

std::tuple<uint64_t, cv::Mat, std::string, object_detect_result_list, std::vector<float>> ThreadSafeQueue::get_latest_inference() {
    std::unique_lock<std::mutex> lock(mutex_);

    if (!inference_queue.empty()) {
        auto latest = inference_queue.rbegin();  // 获取最新元素（序号最大的元素）

        // 在锁的保护下完成深拷贝
        uint64_t seq_num = latest->first;
        cv::Mat img = std::get<cv::Mat>(latest->second).clone();  // 深拷贝图像
        std::string json_result = std::get<std::string>(latest->second);  // 拷贝 JSON 字符串
        object_detect_result_list result_list = std::get<object_detect_result_list>(latest->second);  // 拷贝推理结果结构体
		std::vector<float> position = std::get<std::vector<float>>(latest->second);  // 舵机位置

        lock.unlock();  // 完成深拷贝后释放锁

        // 返回深拷贝的结果
        return { seq_num, img, json_result, result_list, position };
    }

    return { 0, cv::Mat(), "", {}, {} };  // 如果队列为空，返回空值
}