// Server.h
#ifndef SERVER_H
#define SERVER_H

// 包含必要的头文件
#include <iostream> // 标准输入输出流库
#include <string> // 字符串库
#include <chrono> // 时间库
#include <ctime> // C风格时间库

// 定义每个WebSocket连接的用户数据结构
struct PerSocketData {
    std::string connectionTime;  // 连接建立时间
    std::string ipAddress;       // 用户IP地址
};

#endif // SERVER_H