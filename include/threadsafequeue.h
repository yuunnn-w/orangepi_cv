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
        // ���ԭʼͼ������
        void push_raw(const cv::Mat& img);

        // ���������
        void push_inference(const cv::Mat& img, const std::string& result);

        // ��ȡ�����е�����ԭʼͼ�����ݣ������
        cv::Mat get_latest_raw();

        // ��ȡ�������е�����Ԫ�أ������
        std::pair<cv::Mat, std::string> get_latest_inference();

    private:
        std::queue<cv::Mat> raw_queue;
        std::queue<std::pair<cv::Mat, std::string>> inference_queue;
        std::mutex mutex_;  // �������в����Ļ�����
};

extern ThreadSafeQueue image_queue; // ȫ�ֱ���

#endif // THREAD_SAFE_QUEUE_H
