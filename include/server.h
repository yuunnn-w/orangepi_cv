// Server.h
#ifndef SERVER_H
#define SERVER_H

// 包含必要的头文件
#include <App.h> // 包含uWebSockets库的App类
#include <iostream> // 标准输入输出流库
#include <string> // 字符串库
#include <chrono> // 时间库
#include <ctime> // C风格时间库

// 定义每个WebSocket连接的用户数据结构
struct PerSocketData {
    std::string connectionTime;  // 连接建立时间
    std::string ipAddress;       // 用户IP地址
};

// WebSocketServer类的声明
class WebSocketServer {
public:
    WebSocketServer(); // 构造函数
    void run(); // 启动服务器的方法

private:
    uWS::App app; // uWebSockets的应用实例

    void setupRoutes(); // 设置路由的方法
    void handleOpen(uWS::WebSocket<false, true, PerSocketData>* ws); // 处理连接打开的方法
    void handleMessage(uWS::WebSocket<false, true, PerSocketData>* ws, std::string_view message, uWS::OpCode opCode); // 处理消息接收的方法
    void handleClose(uWS::WebSocket<false, true, PerSocketData>* ws, int code, std::string_view message); // 处理连接关闭的方法
};

#endif // SERVER_H