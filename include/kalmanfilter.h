#ifndef _KALMANFILTER_H_
#define _KALMANFILTER_H_

#include <iostream>
#include <Eigen/Dense> // 使用Eigen库进行矩阵运算
//#include <Eigen/Sparse> // 使用Eigen库进行稀疏矩阵运算
#include <vector>

class KalmanFilter {
public:
    // 构造函数：初始化状态向量、状态转移矩阵、观测矩阵、过程噪声和观测噪声
    KalmanFilter();

    void init(double x, double y, double v_x = 0, double v_y = 0, double a_x = 0, double a_y = 0); // 初始化状态向量

    void update(double x_measurement, double y_measurement); // 更新状态：传入观测值 (x, y)

	std::vector<double> getSmoothedState(); // 返回平滑后的状态值，不暴露 Eigen 数据类型

    // 返回预测的下一时刻位置值，不暴露 Eigen 数据类型
    std::vector<double> predictNextPosition();

private:
    Eigen::VectorXd x_; // 状态向量 [x, y, v_x, v_y, a_x, a_y]
    Eigen::MatrixXd F_; // 状态转移矩阵
    Eigen::MatrixXd H_; // 观测矩阵
    Eigen::MatrixXd Q_; // 过程噪声协方差矩阵
    Eigen::MatrixXd R_; // 观测噪声协方差矩阵
    Eigen::MatrixXd P_; // 状态协方差矩阵
};

#endif