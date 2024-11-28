// Server.cpp
#include "server.h"

// 构造函数，初始化WebSocketServer对象时会调用setupRoutes函数
WebSocketServer::WebSocketServer() {
    setupRoutes(); // 定义WebSocket服务器的路由
}

// 设置WebSocket服务器的路由
void WebSocketServer::setupRoutes() {
    // 为根路径"/*"设置WebSocket路由
    app.ws<PerSocketData>("/*", {
        /* Settings */
        .compression = uWS::SHARED_COMPRESSOR, // 启用共享压缩器
        .maxPayloadLength = 16 * 1024, // 最大消息负载长度为16KB
        .idleTimeout = 10, // 空闲超时时间为10秒
        .maxBackpressure = 1 * 1024 * 1024, // 最大背压为1MB
        .closeOnBackpressureLimit = false, // 达到背压限制时不关闭连接
        .resetIdleTimeoutOnSend = true, // 发送消息时重置空闲超时
        .sendPingsAutomatically = true, // 自动发送Ping消息以保持连接活跃
        /* Handlers */
        .open = [this](auto* ws) { handleOpen(ws); }, // 连接打开时的处理函数
        .message = [this](auto* ws, std::string_view message, uWS::OpCode opCode) { handleMessage(ws, message, opCode); }, // 接收到消息时的处理函数
        .close = [this](auto* ws, int code, std::string_view message) { handleClose(ws, code, message); } // 连接关闭时的处理函数
    }).ws<PerSocketData>("/help", {
        /* Settings */
        .compression = uWS::SHARED_COMPRESSOR, // 启用共享压缩器
        .maxPayloadLength = 16 * 1024, // 最大消息负载长度为16KB
        .idleTimeout = 10, // 空闲超时时间为10秒
        .maxBackpressure = 1 * 1024 * 1024, // 最大背压为1MB
        .closeOnBackpressureLimit = false, // 达到背压限制时不关闭连接
        .resetIdleTimeoutOnSend = true, // 发送消息时重置空闲超时
        .sendPingsAutomatically = true, // 自动发送Ping消息以保持连接活跃
        /* Handlers */
        .open = [this](auto* ws) { handleOpen(ws); }, // 连接打开时的处理函数
        .message = [this](auto* ws, std::string_view message, uWS::OpCode opCode) { handleMessage(ws, message, opCode); }, // 接收到消息时的处理函数
        .close = [this](auto* ws, int code, std::string_view message) { handleClose(ws, code, message); } // 连接关闭时的处理函数
    }).ws<PerSocketData>("/tool", {
        /* Settings */
        .compression = uWS::SHARED_COMPRESSOR, // 启用共享压缩器
        .maxPayloadLength = 16 * 1024, // 最大消息负载长度为16KB
        .idleTimeout = 10, // 空闲超时时间为10秒
        .maxBackpressure = 1 * 1024 * 1024, // 最大背压为1MB
        .closeOnBackpressureLimit = false, // 达到背压限制时不关闭连接
        .resetIdleTimeoutOnSend = true, // 发送消息时重置空闲超时
        .sendPingsAutomatically = true, // 自动发送Ping消息以保持连接活跃
        /* Handlers */
        .open = [this](auto* ws) { handleOpen(ws); }, // 连接打开时的处理函数
        .message = [this](auto* ws, std::string_view message, uWS::OpCode opCode) { handleMessage(ws, message, opCode); }, // 接收到消息时的处理函数
        .close = [this](auto* ws, int code, std::string_view message) { handleClose(ws, code, message); } // 连接关闭时的处理函数
    });
}

// 处理WebSocket连接打开事件
void WebSocketServer::handleOpen(uWS::WebSocket<false, true, PerSocketData>* ws) {
    auto now = std::chrono::system_clock::now(); // 获取当前时间
    std::time_t now_time = std::chrono::system_clock::to_time_t(now); // 将当前时间转换为time_t类型
    std::tm* now_tm = std::localtime(&now_time); // 将time_t类型的时间转换为本地时间
    char time_str[20];
    std::strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", now_tm); // 将时间格式化为字符串
    std::string ipAddress = std::string(ws->getRemoteAddressAsText()); // 获取客户端的IP地址
    PerSocketData* userData = ws->getUserData(); // 获取连接的用户数据
    userData->connectionTime = time_str; // 记录连接时间
    userData->ipAddress = ipAddress; // 记录客户端IP地址

    ws->send("Hello, World!", uWS::OpCode::TEXT); // 向客户端发送欢迎消息
}

// 处理WebSocket消息接收事件
void WebSocketServer::handleMessage(uWS::WebSocket<false, true, PerSocketData>* ws, std::string_view message, uWS::OpCode opCode) {
    PerSocketData* userData = (PerSocketData*)ws->getUserData(); // 获取连接的用户数据
    std::cout << "Message received from client. Connection time: " << userData->connectionTime
        << ", IP address: " << userData->ipAddress << std::endl; // 输出接收到的消息信息
    ws->send(message, opCode); // 将接收到的消息原样发送回客户端
}

// 处理WebSocket连接关闭事件
void WebSocketServer::handleClose(uWS::WebSocket<false, true, PerSocketData>* ws, int code, std::string_view message) {
    PerSocketData* userData = (PerSocketData*)ws->getUserData(); // 获取连接的用户数据
    std::cout << "WebSocket connection closed with code: " << code
        << " and message: " << message
        << ". Connection time: " << userData->connectionTime
        << ", IP address: " << userData->ipAddress << std::endl; // 输出连接关闭的信息
}

// 启动WebSocket服务器
void WebSocketServer::run() {
    app.listen(9001, [](auto* listen_socket) {
        if (listen_socket) {
            std::cout << "Listening on port " << 9001 << std::endl; // 成功监听端口9001
        }
        else {
            std::cout << "Failed to listen on port 9001" << std::endl; // 监听端口9001失败
        }
    }).run(); // 启动服务器
}