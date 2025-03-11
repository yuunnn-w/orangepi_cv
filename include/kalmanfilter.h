#ifndef KALMAN_FILTER_H
#define KALMAN_FILTER_H

#include <vector>
#include <Eigen/Dense>

class KalmanFilter {
public:
    /**
     * @brief ���캯������ʼ���˲�����������
     * @param dt ʱ�䲽��
     * @param q ������������������6ά����Ӧ6��״̬������
     * @param r �۲���������������2ά����Ӧ��x�ͦ�y��
     */
    KalmanFilter(double dt, const std::vector<double>& q, const std::vector<double>& r);

    /**
     * @brief ��ʼ��״̬������Э�������
     * @param initial_state ��ʼ״̬������6ά����x, ��y, ��x, ��y, ��x, ��y��
     * @param initial_P ��ʼЭ�������Խ���Ԫ�أ�6ά��
     */
    void Init(const std::vector<double>& initial_state, const std::vector<double>& initial_P);

    /**
     * @brief ִ��Ԥ�ⲽ��
     * @return Ԥ��ĽǶ�ֵ����x, ��y��
     */
    std::vector<double> Predict();

    /**
     * @brief ִ�и��²���
     * @param z �۲�ֵ����x, ��y��
     * @return ���º�����Ź��ƽǶ�ֵ����x, ��y��
     */
    std::vector<double> Update(const std::vector<double>& z);

private:
    // ״̬���� [��x, ��y, ��x, ��y, ��x, ��y]
    Eigen::VectorXd x;

    // ���Э�������
    Eigen::MatrixXd P;

    // �۲���󣨴�״̬����ȡ��x�ͦ�y��
    Eigen::MatrixXd H;

    // ״̬ת�ƾ���
    Eigen::MatrixXd F;

    // ��������Э�������
    Eigen::MatrixXd Q;

    // ��������Э�������
    Eigen::MatrixXd R;

    // ʱ�䲽��
    double dt;
};

#endif // KALMAN_FILTER_H
