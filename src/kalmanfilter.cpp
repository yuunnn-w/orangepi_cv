#include "kalmanfilter.h"

KalmanFilter::KalmanFilter(double dt, const std::vector<double>& q, const std::vector<double>& r) : dt(dt) {
    // 初始化状态转移矩阵F（6x6）
    F = Eigen::MatrixXd::Identity(6, 6);
    F(0, 2) = dt;         // θx 的更新与ωx相关
    F(0, 4) = 0.5 * dt * dt;  // θx 的更新与αx相关
    F(1, 3) = dt;         // θy 的更新与ωy相关
    F(1, 5) = 0.5 * dt * dt;  // θy 的更新与αy相关
    F(2, 4) = dt;         // ωx 的更新与αx相关
    F(3, 5) = dt;         // ωy 的更新与αy相关

    // 初始化观测矩阵H（2x6）
    H = Eigen::MatrixXd::Zero(2, 6);
    H(0, 0) = 1;  // 观测θx
    H(1, 1) = 1;  // 观测θy

    // 初始化过程噪声协方差矩阵Q（6x6对角矩阵）
    Eigen::VectorXd q_diag = Eigen::VectorXd::Map(q.data(), q.size());
    Q = q_diag.asDiagonal();

    // 初始化测量噪声协方差矩阵R（2x2对角矩阵）
    Eigen::VectorXd r_diag = Eigen::VectorXd::Map(r.data(), r.size());
    R = r_diag.asDiagonal();
}

void KalmanFilter::Init(const std::vector<double>& initial_state, const std::vector<double>& initial_P) {
    // 初始化状态向量
    x = Eigen::VectorXd::Map(initial_state.data(), initial_state.size());

    // 初始化协方差矩阵（对角线元素）
    Eigen::VectorXd p_diag = Eigen::VectorXd::Map(initial_P.data(), initial_P.size());
    P = p_diag.asDiagonal();
}

std::vector<double> KalmanFilter::Predict() {
    // 执行状态预测
    x = F * x;

    // 计算先验协方差矩阵
    P = F * P * F.transpose() + Q;

    // 返回预测的角度值
    return { x(0), x(1) };
}

std::vector<double> KalmanFilter::Update(const std::vector<double>& z) {
    // 将观测值转换为Eigen向量
    Eigen::Vector2d z_eigen(z[0], z[1]);

    // 计算观测残差
    Eigen::Vector2d y = z_eigen - H * x;

    // 计算残差协方差
    Eigen::Matrix2d S = H * P * H.transpose() + R;

    // 计算卡尔曼增益
    Eigen::MatrixXd K = P * H.transpose() * S.inverse();

    // 更新状态估计
    x = x + K * y;

    // 更新协方差矩阵
    Eigen::MatrixXd I = Eigen::MatrixXd::Identity(6, 6);
    P = (I - K * H) * P;

    // 返回更新后的角度值
    return { x(0), x(1) };
}