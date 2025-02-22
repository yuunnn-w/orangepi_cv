// kalmanfilter.cpp
#include "kalmanfilter.h"

KalmanFilter::KalmanFilter() {
    // ״̬������x, y, v_x, v_y, a_x, a_y
    x_ = Eigen::VectorXd::Zero(6);

    // ״̬ת�ƾ��� F (����ʱ�䲽��Ϊ1)
    F_ = Eigen::MatrixXd::Identity(6, 6);
    F_(0, 2) = 1; // x += v_x
    F_(1, 3) = 1; // y += v_y
    F_(2, 4) = 1; // v_x += a_x
    F_(3, 5) = 1; // v_y += a_y

    // �۲���� H (ֻ�ܹ۲⵽x, y)
    H_ = Eigen::MatrixXd::Zero(2, 6);
    H_(0, 0) = 1; // �۲�x
    H_(1, 1) = 1; // �۲�y

    // ��������Э������� Q
    Q_ = Eigen::MatrixXd::Identity(6, 6) * 0.1;

    // �۲�����Э������� R
    R_ = Eigen::MatrixXd::Identity(2, 2) * 1;

    // ״̬Э������� P
    P_ = Eigen::MatrixXd::Identity(6, 6);
}

void KalmanFilter::init(double x, double y, double v_x, double v_y, double a_x, double a_y) {
    x_ << x, y, v_x, v_y, a_x, a_y;
}

// ����״̬������۲�ֵ (x, y)
void KalmanFilter::update(double x_measurement, double y_measurement) {
    // ���۲�ֵת��Ϊ����
    Eigen::Vector2d measurement;
    measurement << x_measurement, y_measurement;

    // ���㿨��������
    Eigen::MatrixXd K = P_ * H_.transpose() * (H_ * P_ * H_.transpose() + R_).inverse();

    // ����״̬
    x_ = x_ + K * (measurement - H_ * x_);

    // ����Э����
    P_ = (Eigen::MatrixXd::Identity(6, 6) - K * H_) * P_;
}

std::vector<double> KalmanFilter::getSmoothedState() {
    std::vector<double> state;
    state.reserve(6); // Ԥ����ռ�
    for (int i = 0; i < 6; ++i) {
        state.push_back(x_(i)); // �� Eigen::VectorXd ת��Ϊ std::vector<double>
    }
    return state;
}

std::vector<double> KalmanFilter::predictNextPosition() {
    Eigen::VectorXd next_state = F_ * x_;  // ʹ��״̬ת�ƾ��� F Ԥ����һʱ�̵�״̬
    return { next_state(0), next_state(1) }; // ����Ԥ���λ�� (x, y)
}