#include "kalmanfilter.h"

KalmanFilter::KalmanFilter(double dt, const std::vector<double>& q, const std::vector<double>& r) : dt(dt) {
    // ��ʼ��״̬ת�ƾ���F��6x6��
    F = Eigen::MatrixXd::Identity(6, 6);
    F(0, 2) = dt;         // ��x �ĸ������x���
    F(0, 4) = 0.5 * dt * dt;  // ��x �ĸ������x���
    F(1, 3) = dt;         // ��y �ĸ������y���
    F(1, 5) = 0.5 * dt * dt;  // ��y �ĸ������y���
    F(2, 4) = dt;         // ��x �ĸ������x���
    F(3, 5) = dt;         // ��y �ĸ������y���

    // ��ʼ���۲����H��2x6��
    H = Eigen::MatrixXd::Zero(2, 6);
    H(0, 0) = 1;  // �۲��x
    H(1, 1) = 1;  // �۲��y

    // ��ʼ����������Э�������Q��6x6�ԽǾ���
    Eigen::VectorXd q_diag = Eigen::VectorXd::Map(q.data(), q.size());
    Q = q_diag.asDiagonal();

    // ��ʼ����������Э�������R��2x2�ԽǾ���
    Eigen::VectorXd r_diag = Eigen::VectorXd::Map(r.data(), r.size());
    R = r_diag.asDiagonal();
}

void KalmanFilter::Init(const std::vector<double>& initial_state, const std::vector<double>& initial_P) {
    // ��ʼ��״̬����
    x = Eigen::VectorXd::Map(initial_state.data(), initial_state.size());

    // ��ʼ��Э������󣨶Խ���Ԫ�أ�
    Eigen::VectorXd p_diag = Eigen::VectorXd::Map(initial_P.data(), initial_P.size());
    P = p_diag.asDiagonal();
}

std::vector<double> KalmanFilter::Predict() {
    // ִ��״̬Ԥ��
    x = F * x;

    // ��������Э�������
    P = F * P * F.transpose() + Q;

    // ����Ԥ��ĽǶ�ֵ
    return { x(0), x(1) };
}

std::vector<double> KalmanFilter::Update(const std::vector<double>& z) {
    // ���۲�ֵת��ΪEigen����
    Eigen::Vector2d z_eigen(z[0], z[1]);

    // ����۲�в�
    Eigen::Vector2d y = z_eigen - H * x;

    // ����в�Э����
    Eigen::Matrix2d S = H * P * H.transpose() + R;

    // ���㿨��������
    Eigen::MatrixXd K = P * H.transpose() * S.inverse();

    // ����״̬����
    x = x + K * y;

    // ����Э�������
    Eigen::MatrixXd I = Eigen::MatrixXd::Identity(6, 6);
    P = (I - K * H) * P;

    // ���ظ��º�ĽǶ�ֵ
    return { x(0), x(1) };
}