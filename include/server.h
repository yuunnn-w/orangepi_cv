// Server.h
#ifndef SERVER_H
#define SERVER_H

// ������Ҫ��ͷ�ļ�
#include <iostream> // ��׼�����������
#include <string> // �ַ�����
#include <chrono> // ʱ���
#include <ctime> // C���ʱ���

// ����ÿ��WebSocket���ӵ��û����ݽṹ
struct PerSocketData {
    std::string connectionTime;  // ���ӽ���ʱ��
    std::string ipAddress;       // �û�IP��ַ
};

#endif // SERVER_H