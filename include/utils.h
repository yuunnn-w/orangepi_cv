#ifndef UTILS_H
#define UTILS_H

#include "rknn_model.h"
#include "threadsafequeue.h"
#include "servo_driver.h"
#include "kalmanfilter.h"

#include <atomic>
#include <nlohmann/json.hpp>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <utility> // ���� std::pair
#include <cmath>


using json = nlohmann::json;

// sudo apt install nlohmann-json3-dev
extern std::atomic<bool> stop_inference;
extern std::atomic<bool> system_sleep;
extern std::atomic<bool> poweroff;
extern std::atomic<bool> is_sleeping;

extern std::mutex mtx; // ������
extern std::mutex servo_mtx; // ���������
extern std::atomic<bool> new_image_available;

// Ŀ�����ĺ궨��
#define TARGET_CLASS_ID 67 // Ŀ������ ID


void inference_images(rknn_model& model, int ctx_index); // ������
std::pair<int, int> getTargetCenter(const object_detect_result_list& od_results); // ��ȡĿ�����ĵ�һ��Ŀ�����������
void calculateObjectAngles(float servoXAngle, float servoYAngle, int u, int v, float& thetaX, float& thetaY); // ���������ڶ������ϵ�еĽǶ�����
void control_servo(); // ���ƶ��

void show_inference(); // ��ʾ������

#endif