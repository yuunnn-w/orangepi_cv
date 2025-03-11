// bytetrack.h

#ifndef BYTE_TRACK_H
#define BYTE_TRACK_H

#include <iostream>
#include <vector>
#include <cmath>
#include "kalmanfilter.h"

// 跟踪器类定义
class ByteTracker {
private:
    int lost_count_ = 0;
    bool is_active_ = false;
    float last_valid_thetaX_ = 0;
    float last_valid_thetaY_ = 0;

    // 可配置参数
    const int max_lost_ = 15;         // 最大丢失帧数
    const double dt = 0.03; // 时间间隔
	std::vector<double> q = { 0.1, 0.1, 0.5, 0.5, 1.0, 1.0 };//{ θx, θy, ωx, ωy, αx, αx }; // 过程噪声方差，角度、角速度、角加速度
	std::vector<double> r = { 0.05, 0.05 }; // 测量噪声方差，只能观测角度θx, θy
    const double match_threshold_ = 10.0; // 角度匹配阈值(度)
    
	// Kalman滤波器
    KalmanFilter kf_;
	std::vector<double> estimate; // 平滑后的当前角度值


public:
    ByteTracker() : kf_(dt, q, r) {} // 初始化 KalmanFilter

    void initialize(float thetaX, float thetaY) {
        // 初始状态: [θx, θy, Δθx, Δθy]
		std::vector<double> init_state = { thetaX, thetaY, 0.0, 0.0, 0.0, 0.0 }; // 初始状态
        std::vector<double> init_P = { 1.0, 1.0, 1.0, 1.0, 1.0, 1.0 }; // 初始协方差

        kf_.Init(init_state, init_P);
        estimate = { thetaX, thetaY }; // 初始化估计值

        lost_count_ = 0;
        is_active_ = true;
        last_valid_thetaX_ = thetaX;
        last_valid_thetaY_ = thetaY;
    }

    std::vector<double> update(float thetaX, float thetaY) {
        std::vector<double> z = { thetaX, thetaY };
		estimate = kf_.Update(z); // 更新状态，返回滤波后当前状态的估计值
		last_valid_thetaX_ = thetaX; // 更新原始观测的的角度值
		last_valid_thetaY_ = thetaY;
		return estimate;
    }

	std::vector<double> predict() { // 预测下一步状态

        std::vector<double> p = kf_.Predict();
		return p;
    }
	void mark_loss() { // 标记丢失
		if (is_active_) {
			// std::cout << "Tracking lost." << std::endl;
            lost_count_++;
			// 打印当前丢失帧数
			// std::cout << "Lost count: " << lost_count_ << std::endl;
            if (lost_count_ > max_lost_) {
                is_active_ = false;
				std::cout << "Tracking lost for too long, deactivating." << std::endl;
            }
		}
        else {
			std::cout << "Tracking already lost." << std::endl;
        }
    }

    std::pair<float, float> get_current_angles() const {
        return { static_cast<float>(estimate[0]),
				static_cast<float>(estimate[1]) }; // 返回平滑后的当前角度值
    }

    bool is_active() const { return is_active_; }
    bool needs_activation() const { return !is_active_; }

    // 角度距离匹配函数
    bool is_match(float detect_thetaX, float detect_thetaY) const {
        auto [track_thetaX, track_thetaY] = get_current_angles();
        double dx = detect_thetaX - track_thetaX;
        double dy = detect_thetaY - track_thetaY;
        return std::sqrt(dx * dx + dy * dy) <= match_threshold_;
    }
};


#endif // BYTE_TRACK_H