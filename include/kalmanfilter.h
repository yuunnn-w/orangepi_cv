#ifndef _KALMANFILTER_H_
#define _KALMANFILTER_H_

#include <iostream>
#include <Eigen/Dense> // ʹ��Eigen����о�������
//#include <Eigen/Sparse> // ʹ��Eigen�����ϡ���������
#include <vector>

class KalmanFilter {
public:
    // ���캯������ʼ��״̬������״̬ת�ƾ��󡢹۲���󡢹��������͹۲�����
    KalmanFilter();

    void init(double x, double y, double v_x = 0, double v_y = 0, double a_x = 0, double a_y = 0); // ��ʼ��״̬����

    void update(double x_measurement, double y_measurement); // ����״̬������۲�ֵ (x, y)

	std::vector<double> getSmoothedState(); // ����ƽ�����״ֵ̬������¶ Eigen ��������

    // ����Ԥ�����һʱ��λ��ֵ������¶ Eigen ��������
    std::vector<double> predictNextPosition();

private:
    Eigen::VectorXd x_; // ״̬���� [x, y, v_x, v_y, a_x, a_y]
    Eigen::MatrixXd F_; // ״̬ת�ƾ���
    Eigen::MatrixXd H_; // �۲����
    Eigen::MatrixXd Q_; // ��������Э�������
    Eigen::MatrixXd R_; // �۲�����Э�������
    Eigen::MatrixXd P_; // ״̬Э�������
};

#endif