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
extern std::atomic<bool> allow_auto_control; // �Ƿ�������������
extern std::atomic<bool> camera_mode;

extern std::mutex mtx; // ������
extern std::mutex servo_mtx; // ���������
extern std::atomic<bool> new_image_available;

extern std::atomic<float> targetX; // ȫ��Ŀ��λ�ñ���X
extern std::atomic<float> targetY; // ȫ��Ŀ��λ�ñ���Y

// ȫ�ֶ���Ƕȱ���
extern std::atomic<float> ServoXAngle; // ���X��Ƕ�
extern std::atomic<float> ServoYAngle; // ���Y��Ƕ�

// �Ƿ�ʶ��Ŀ��
extern std::atomic<bool> target_detected; // ��־��������ʾ�Ƿ��⵽Ŀ��

// ���������˶���Χ
const float SERVO1_MIN_ANGLE = -125.0;
const float SERVO1_MAX_ANGLE = 125.0;
const float SERVO2_MIN_ANGLE = -65.0;
const float SERVO2_MAX_ANGLE = 80.0;

// ��ֵ���ã�����ʵ�ʻ���������
const double DARK_THRESHOLD = 50.0;    // �к�����ֵ�����ڴ�ֵ������
const double LIGHT_THRESHOLD = 80.0;   // �пɼ�����ֵ�����ڴ�ֵ������
const int SWITCH_FRAMES = 5;           // ����N֡�����л�����������

// �Լ����ڵ�λ��
const float SELF_X = 0.0; // �Լ���X����
const float SELF_Y = 0.22; // �Լ���Y����


// Ŀ�����ĺ궨��
#define TARGET_CLASS_ID 67 // Ŀ������ ID


void inference_images(rknn_model& model, int ctx_index); // ������
std::pair<int, int> getTargetCenter(const object_detect_result_list& od_results); // ��ȡĿ�����ĵ�һ��Ŀ�����������
void calculateObjectAngles(float servoXAngle, float servoYAngle, int u, int v, float& thetaX, float& thetaY); // ���������ڶ������ϵ�еĽǶ�����
void control_servo(); // ���ƶ��

void show_inference(); // ��ʾ������

#endif