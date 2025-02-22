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

using json = nlohmann::json;

// sudo apt install nlohmann-json3-dev
extern std::atomic<bool> stop_inference;
extern std::atomic<bool> system_sleep;
extern std::atomic<bool> poweroff;
extern std::atomic<bool> is_sleeping;

extern std::mutex mtx;
extern std::atomic<bool> new_image_available;



void inference_images(rknn_model& model, int ctx_index);

void control_servo(ServoDriver& servoDriver, int servo1, int servo2);

void show_inference();

#endif