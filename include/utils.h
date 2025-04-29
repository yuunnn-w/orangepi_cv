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
extern std::atomic<bool> allow_auto_control; // 是否允许自主控制
extern std::atomic<bool> camera_mode;

extern std::mutex mtx; // 互斥锁
extern std::mutex servo_mtx; // 舵机互斥锁
extern std::atomic<bool> new_image_available;

extern std::atomic<float> targetX; // 全局目标位置变量X
extern std::atomic<float> targetY; // 全局目标位置变量Y

// 全局舵机角度变量
extern std::atomic<float> ServoXAngle; // 舵机X轴角度
extern std::atomic<float> ServoYAngle; // 舵机Y轴角度

// 是否识别到目标
extern std::atomic<bool> target_detected; // 标志变量，表示是否检测到目标

// 定义舵机的运动范围
const float SERVO1_MIN_ANGLE = -125.0;
const float SERVO1_MAX_ANGLE = 125.0;
const float SERVO2_MIN_ANGLE = -65.0;
const float SERVO2_MAX_ANGLE = 80.0;

// 阈值配置（根据实际环境调整）
const double DARK_THRESHOLD = 50.0;    // 切红外阈值（低于此值触发）
const double LIGHT_THRESHOLD = 80.0;   // 切可见光阈值（高于此值触发）
const int SWITCH_FRAMES = 5;           // 连续N帧达标才切换（防抖动）

// 自己所在的位置
const float SELF_X = 0.0; // 自己的X坐标
const float SELF_Y = 0.22; // 自己的Y坐标


// 目标类别的宏定义
#define TARGET_CLASS_ID 67 // 目标类别的 ID


void inference_images(rknn_model& model, int ctx_index); // 推理函数
std::pair<int, int> getTargetCenter(const object_detect_result_list& od_results); // 获取目标类别的第一个目标的中心坐标
void calculateObjectAngles(float servoXAngle, float servoYAngle, int u, int v, float& thetaX, float& thetaY); // 计算物体在舵机坐标系中的角度坐标
void control_servo(); // 控制舵机

void show_inference(); // 显示推理结果

#endif