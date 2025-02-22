// kalmanfilter.cpp
#include "kalmanfilter.h"

KalmanFilter::KalmanFilter() {
    // 状态向量：x, y, v_x, v_y, a_x, a_y
    x_ = Eigen::VectorXd::Zero(6);

    // 状态转移矩阵 F (假设时间步长为1)
    F_ = Eigen::MatrixXd::Identity(6, 6);
    F_(0, 2) = 1; // x += v_x
    F_(1, 3) = 1; // y += v_y
    F_(2, 4) = 1; // v_x += a_x
    F_(3, 5) = 1; // v_y += a_y

    // 观测矩阵 H (只能观测到x, y)
    H_ = Eigen::MatrixXd::Zero(2, 6);
    H_(0, 0) = 1; // 观测x
    H_(1, 1) = 1; // 观测y

    // 过程噪声协方差矩阵 Q
    Q_ = Eigen::MatrixXd::Identity(6, 6) * 0.1;

    // 观测噪声协方差矩阵 R
    R_ = Eigen::MatrixXd::Identity(2, 2) * 1;

    // 状态协方差矩阵 P
    P_ = Eigen::MatrixXd::Identity(6, 6);
}

void KalmanFilter::init(double x, double y, double v_x, double v_y, double a_x, double a_y) {
    x_ << x, y, v_x, v_y, a_x, a_y;
}

// 更新状态：传入观测值 (x, y)
void KalmanFilter::update(double x_measurement, double y_measurement) {
    // 将观测值转换为向量
    Eigen::Vector2d measurement;
    measurement << x_measurement, y_measurement;

    // 计算卡尔曼增益
    Eigen::MatrixXd K = P_ * H_.transpose() * (H_ * P_ * H_.transpose() + R_).inverse();

    // 更新状态
    x_ = x_ + K * (measurement - H_ * x_);

    // 更新协方差
    P_ = (Eigen::MatrixXd::Identity(6, 6) - K * H_) * P_;
}

std::vector<double> KalmanFilter::getSmoothedState() {
    std::vector<double> state;
    state.reserve(6); // 预分配空间
    for (int i = 0; i < 6; ++i) {
        state.push_back(x_(i)); // 将 Eigen::VectorXd 转换为 std::vector<double>
    }
    return state;
}

std::vector<double> KalmanFilter::predictNextPosition() {
    Eigen::VectorXd next_state = F_ * x_;  // 使用状态转移矩阵 F 预测下一时刻的状态
    return { next_state(0), next_state(1) }; // 返回预测的位置 (x, y)
}