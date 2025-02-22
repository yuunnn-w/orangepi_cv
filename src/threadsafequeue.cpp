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

    // ������г�����󳤶ȣ�ɾ�������Ԫ��
    if (raw_queue.size() >= max_raw_size_) {
        raw_queue.pop_front();
    }
    // std::cerr << "New Frame: " << seq_num << std::endl;
    // ����ͼ���������
    raw_queue.push_back({ img.clone(), seq_num });
}

void ThreadSafeQueue::push_inference(uint64_t sequence_number, const cv::Mat& img, const std::string& json_result, const object_detect_result_list& result_list) {
    std::unique_lock<std::mutex> lock(mutex_);

    // ������������Ų��뵽��ȷ��λ��
    inference_queue[sequence_number] = { img, json_result, result_list };

    // ������г�����󳤶ȣ�ɾ����ɵ�Ԫ�أ������С��Ԫ�أ�
    if (inference_queue.size() > max_inference_size_) {
        inference_queue.erase(inference_queue.begin());
    }

    // ����֡��ͳ��
    update_fps();
}

std::pair<uint64_t, cv::Mat> ThreadSafeQueue::get_latest_raw() {
    std::unique_lock<std::mutex> lock(mutex_);

    if (!raw_queue.empty()) {
        auto latest = raw_queue.back();  // ��ȡ����Ԫ��

        // �����ı�����������
        uint64_t seq_num = latest.second;
        cv::Mat img = latest.first;  // ���ͼ��

        raw_queue.pop_back();  // ɾ������Ԫ��
        lock.unlock();         // �ͷ���

        // ��������Ľ��
        return { seq_num, img };
    }

    return { 0, cv::Mat() };  // �������Ϊ�գ����ؿյ� cv::Mat ����� 0
}

std::tuple<uint64_t, cv::Mat, std::string, object_detect_result_list> ThreadSafeQueue::get_latest_inference() {
    std::unique_lock<std::mutex> lock(mutex_);

    if (!inference_queue.empty()) {
        auto latest = inference_queue.rbegin();  // ��ȡ����Ԫ�أ��������Ԫ�أ�

        // �����ı�����������
        uint64_t seq_num = latest->first;
        cv::Mat img = std::get<0>(latest->second).clone();  // ���ͼ��
        std::string json_result = std::get<1>(latest->second);  // ���� JSON �ַ���
        object_detect_result_list result_list = std::get<2>(latest->second);  // �����������ṹ��

        lock.unlock();  // ���������ͷ���

        // ��������Ľ��
        return { seq_num, img, json_result, result_list };
    }

    return { 0, cv::Mat(), "", {} };  // �������Ϊ�գ����ؿ�ֵ
}