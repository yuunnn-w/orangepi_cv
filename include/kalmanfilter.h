#ifndef KALMAN_FILTER_H
#define KALMAN_FILTER_H

#include <vector>
#include <Eigen/Dense>

class KalmanFilter {
public:
    /**
     * @brief 构造函数，初始化滤波器基本参数
     * @param dt 时间步长
     * @param q 过程噪声方差向量（6维，对应6个状态变量）
     * @param r 观测噪声方差向量（2维，对应θx和θy）
     */
    KalmanFilter(double dt, const std::vector<double>& q, const std::vector<double>& r);

    /**
     * @brief 初始化状态向量和协方差矩阵
     * @param initial_state 初始状态向量（6维：θx, θy, ωx, ωy, αx, αy）
     * @param initial_P 初始协方差矩阵对角线元素（6维）
     */
    void Init(const std::vector<double>& initial_state, const std::vector<double>& initial_P);

    /**
     * @brief 执行预测步骤
     * @return 预测的角度值（θx, θy）
     */
    std::vector<double> Predict();

    /**
     * @brief 执行更新步骤
     * @param z 观测值（θx, θy）
     * @return 更新后的最优估计角度值（θx, θy）
     */
    std::vector<double> Update(const std::vector<double>& z);

private:
    // 状态向量 [θx, θy, ωx, ωy, αx, αy]
    Eigen::VectorXd x;

    // 误差协方差矩阵
    Eigen::MatrixXd P;

    // 观测矩阵（从状态中提取θx和θy）
    Eigen::MatrixXd H;

    // 状态转移矩阵
    Eigen::MatrixXd F;

    // 过程噪声协方差矩阵
    Eigen::MatrixXd Q;

    // 测量噪声协方差矩阵
    Eigen::MatrixXd R;

    // 时间步长
    double dt;
};

#endif // KALMAN_FILTER_H
