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
#include <utility> // 用于 std::pair
#include <cmath>


using json = nlohmann::json;

// sudo apt install nlohmann-json3-dev
extern std::atomic<bool> stop_inference;
extern std::atomic<bool> system_sleep;
extern std::atomic<bool> poweroff;
extern std::atomic<bool> is_sleeping;

extern std::mutex mtx; // 互斥锁
extern std::mutex servo_mtx; // 舵机互斥锁
extern std::atomic<bool> new_image_available;

// 目标类别的宏定义
#define TARGET_CLASS_ID 67 // 目标类别的 ID


void inference_images(rknn_model& model, int ctx_index); // 推理函数
std::pair<int, int> getTargetCenter(const object_detect_result_list& od_results); // 获取目标类别的第一个目标的中心坐标
void calculateObjectAngles(float servoXAngle, float servoYAngle, int u, int v, float& thetaX, float& thetaY); // 计算物体在舵机坐标系中的角度坐标
void control_servo(); // 控制舵机

void show_inference(); // 显示推理结果

#endif