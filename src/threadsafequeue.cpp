// threadsafequeue.cpp
#include "threadsafequeue.h"

ThreadSafeQueue image_queue;

void ThreadSafeQueue::push_raw(const cv::Mat& img) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (img.empty()) {
        std::cerr << "Warning: Attempted to push an empty image in push_raw()" << std::endl;
        return;
    }

    // �������
	uint64_t seq_num = sequence_number_.fetch_add(1, std::memory_order_relaxed);
	// �Ȼ�ȡ��ǰ��ţ�Ȼ���1������ԭ�������

    // ������г�����󳤶ȣ�ɾ�������Ԫ��
    if (raw_queue.size() >= max_raw_size_) {
        raw_queue.pop_front();
    }
    // std::cerr << "New Frame: " << seq_num << std::endl;
    // ��ȡ��ǰ���λ��
    // �������������
	std::unique_lock<std::mutex> servo_lock(servo_mtx);
    float x_angle = servoDriver.getCurrentPosition(1);
    float y_angle = servoDriver.getCurrentPosition(2);
	servo_lock.unlock();
	// std::cout << "Servo1 Angle: " << x_angle << std::endl; //ˮƽת�����
	// std::cout << "Servo2 Angle: " << y_angle << std::endl; //��ֱת�����
    // �������λ�õ�vector
	std::vector<float> servo_position = { x_angle, y_angle };
    // ����ͼ����źͶ��λ�����
    raw_queue.emplace_back(img.clone(), seq_num, servo_position);
}

void ThreadSafeQueue::push_inference(uint64_t sequence_number, const cv::Mat& img, const std::string& json_result, const object_detect_result_list& result_list, const std::vector<float>& position) {
    std::unique_lock<std::mutex> lock(mutex_);

    // ������������Ų��뵽��ȷ��λ��
    inference_queue[sequence_number] = { img, json_result, result_list, position };

    // ������г�����󳤶ȣ�ɾ����ɵ�Ԫ�أ������С��Ԫ�أ�
    if (inference_queue.size() > max_inference_size_) {
        inference_queue.erase(inference_queue.begin());
    }

    // ����֡��ͳ��
    update_fps();
}

std::tuple<uint64_t, cv::Mat, std::vector<float>> ThreadSafeQueue::get_latest_raw() {
    std::unique_lock<std::mutex> lock(mutex_);

    if (!raw_queue.empty()) {
        auto latest = raw_queue.back();  // ��ȡ����Ԫ��

        // �����ı�����������
        uint64_t seq_num = std::get<uint64_t>(latest);
		cv::Mat img = std::get<cv::Mat>(latest).clone();  // ���ͼ��
		std::vector<float> position = std::get<std::vector<float>>(latest);  // ������λ��
        raw_queue.pop_back();  // ɾ������Ԫ��
        lock.unlock();         // �ͷ���

        // ��������Ľ��
        return { seq_num, img, position };
    }

	return { 0, cv::Mat(), {} };  // �������Ϊ�գ����ؿ�ֵ
}

std::tuple<uint64_t, cv::Mat, std::string, object_detect_result_list, std::vector<float>> ThreadSafeQueue::get_latest_inference() {
    std::unique_lock<std::mutex> lock(mutex_);

    if (!inference_queue.empty()) {
        auto latest = inference_queue.rbegin();  // ��ȡ����Ԫ�أ��������Ԫ�أ�

        // �����ı�����������
        uint64_t seq_num = latest->first;
        cv::Mat img = std::get<cv::Mat>(latest->second).clone();  // ���ͼ��
        std::string json_result = std::get<std::string>(latest->second);  // ���� JSON �ַ���
        object_detect_result_list result_list = std::get<object_detect_result_list>(latest->second);  // �����������ṹ��
		std::vector<float> position = std::get<std::vector<float>>(latest->second);  // ���λ��

        lock.unlock();  // ���������ͷ���

        // ��������Ľ��
        return { seq_num, img, json_result, result_list, position };
    }

    return { 0, cv::Mat(), "", {}, {} };  // �������Ϊ�գ����ؿ�ֵ
}