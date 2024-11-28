// Server.h
#ifndef SERVER_H
#define SERVER_H

// ������Ҫ��ͷ�ļ�
#include <App.h> // ����uWebSockets���App��
#include <iostream> // ��׼�����������
#include <string> // �ַ�����
#include <chrono> // ʱ���
#include <ctime> // C���ʱ���

// ����ÿ��WebSocket���ӵ��û����ݽṹ
struct PerSocketData {
    std::string connectionTime;  // ���ӽ���ʱ��
    std::string ipAddress;       // �û�IP��ַ
};

// WebSocketServer�������
class WebSocketServer {
public:
    WebSocketServer(); // ���캯��
    void run(); // �����������ķ���

private:
    uWS::App app; // uWebSockets��Ӧ��ʵ��

    void setupRoutes(); // ����·�ɵķ���
    void handleOpen(uWS::WebSocket<false, true, PerSocketData>* ws); // �������Ӵ򿪵ķ���
    void handleMessage(uWS::WebSocket<false, true, PerSocketData>* ws, std::string_view message, uWS::OpCode opCode); // ������Ϣ���յķ���
    void handleClose(uWS::WebSocket<false, true, PerSocketData>* ws, int code, std::string_view message); // �������ӹرյķ���
};

#endif // SERVER_H