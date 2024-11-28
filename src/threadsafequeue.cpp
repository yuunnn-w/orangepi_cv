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
        raw_queue.pop();  // ������г�����󳤶ȣ�ɾ�������Ԫ��
    }
    raw_queue.push(img);  // ����ͼ�����
}

void ThreadSafeQueue::push_inference(const cv::Mat& img, const std::string& result) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (inference_queue.size() >= 2) {
        inference_queue.pop();  // ������������г�����󳤶ȣ�ɾ�������Ԫ��
    }
    inference_queue.push({ img, result });  // ���������
}

cv::Mat ThreadSafeQueue::get_latest_raw() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!raw_queue.empty()) {
        // cv::Mat latest = raw_queue.back().clone();  // ��������е�����ͼ��
        return raw_queue.back().clone();  // ������������е�����ͼ��
    }
    return cv::Mat();  // �������Ϊ�գ����ؿյ� cv::Mat
}

std::pair<cv::Mat, std::string> ThreadSafeQueue::get_latest_inference() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!inference_queue.empty()) {
        // cv::Mat latest_img = inference_queue.back().first.clone();
        // std::string latest_result = inference_queue.back().second;
        return { inference_queue.back().first.clone(), inference_queue.back().second };  // �������µ��������������
    }
    return { cv::Mat(), "" };  // �������Ϊ�գ����ؿյ� cv::Mat �Ϳյ�������
}
