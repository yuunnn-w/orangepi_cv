// Server.cpp
#include "server.h"

// ���캯������ʼ��WebSocketServer����ʱ�����setupRoutes����
WebSocketServer::WebSocketServer() {
    setupRoutes(); // ����WebSocket��������·��
}

// ����WebSocket��������·��
void WebSocketServer::setupRoutes() {
    // Ϊ��·��"/*"����WebSocket·��
    app.ws<PerSocketData>("/*", {
        /* Settings */
        .compression = uWS::SHARED_COMPRESSOR, // ���ù���ѹ����
        .maxPayloadLength = 16 * 1024, // �����Ϣ���س���Ϊ16KB
        .idleTimeout = 10, // ���г�ʱʱ��Ϊ10��
        .maxBackpressure = 1 * 1024 * 1024, // ���ѹΪ1MB
        .closeOnBackpressureLimit = false, // �ﵽ��ѹ����ʱ���ر�����
        .resetIdleTimeoutOnSend = true, // ������Ϣʱ���ÿ��г�ʱ
        .sendPingsAutomatically = true, // �Զ�����Ping��Ϣ�Ա������ӻ�Ծ
        /* Handlers */
        .open = [this](auto* ws) { handleOpen(ws); }, // ���Ӵ�ʱ�Ĵ�����
        .message = [this](auto* ws, std::string_view message, uWS::OpCode opCode) { handleMessage(ws, message, opCode); }, // ���յ���Ϣʱ�Ĵ�����
        .close = [this](auto* ws, int code, std::string_view message) { handleClose(ws, code, message); } // ���ӹر�ʱ�Ĵ�����
    }).ws<PerSocketData>("/help", {
        /* Settings */
        .compression = uWS::SHARED_COMPRESSOR, // ���ù���ѹ����
        .maxPayloadLength = 16 * 1024, // �����Ϣ���س���Ϊ16KB
        .idleTimeout = 10, // ���г�ʱʱ��Ϊ10��
        .maxBackpressure = 1 * 1024 * 1024, // ���ѹΪ1MB
        .closeOnBackpressureLimit = false, // �ﵽ��ѹ����ʱ���ر�����
        .resetIdleTimeoutOnSend = true, // ������Ϣʱ���ÿ��г�ʱ
        .sendPingsAutomatically = true, // �Զ�����Ping��Ϣ�Ա������ӻ�Ծ
        /* Handlers */
        .open = [this](auto* ws) { handleOpen(ws); }, // ���Ӵ�ʱ�Ĵ�����
        .message = [this](auto* ws, std::string_view message, uWS::OpCode opCode) { handleMessage(ws, message, opCode); }, // ���յ���Ϣʱ�Ĵ�����
        .close = [this](auto* ws, int code, std::string_view message) { handleClose(ws, code, message); } // ���ӹر�ʱ�Ĵ�����
    }).ws<PerSocketData>("/tool", {
        /* Settings */
        .compression = uWS::SHARED_COMPRESSOR, // ���ù���ѹ����
        .maxPayloadLength = 16 * 1024, // �����Ϣ���س���Ϊ16KB
        .idleTimeout = 10, // ���г�ʱʱ��Ϊ10��
        .maxBackpressure = 1 * 1024 * 1024, // ���ѹΪ1MB
        .closeOnBackpressureLimit = false, // �ﵽ��ѹ����ʱ���ر�����
        .resetIdleTimeoutOnSend = true, // ������Ϣʱ���ÿ��г�ʱ
        .sendPingsAutomatically = true, // �Զ�����Ping��Ϣ�Ա������ӻ�Ծ
        /* Handlers */
        .open = [this](auto* ws) { handleOpen(ws); }, // ���Ӵ�ʱ�Ĵ�����
        .message = [this](auto* ws, std::string_view message, uWS::OpCode opCode) { handleMessage(ws, message, opCode); }, // ���յ���Ϣʱ�Ĵ�����
        .close = [this](auto* ws, int code, std::string_view message) { handleClose(ws, code, message); } // ���ӹر�ʱ�Ĵ�����
    });
}

// ����WebSocket���Ӵ��¼�
void WebSocketServer::handleOpen(uWS::WebSocket<false, true, PerSocketData>* ws) {
    auto now = std::chrono::system_clock::now(); // ��ȡ��ǰʱ��
    std::time_t now_time = std::chrono::system_clock::to_time_t(now); // ����ǰʱ��ת��Ϊtime_t����
    std::tm* now_tm = std::localtime(&now_time); // ��time_t���͵�ʱ��ת��Ϊ����ʱ��
    char time_str[20];
    std::strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", now_tm); // ��ʱ���ʽ��Ϊ�ַ���
    std::string ipAddress = std::string(ws->getRemoteAddressAsText()); // ��ȡ�ͻ��˵�IP��ַ
    PerSocketData* userData = ws->getUserData(); // ��ȡ���ӵ��û�����
    userData->connectionTime = time_str; // ��¼����ʱ��
    userData->ipAddress = ipAddress; // ��¼�ͻ���IP��ַ

    ws->send("Hello, World!", uWS::OpCode::TEXT); // ��ͻ��˷��ͻ�ӭ��Ϣ
}

// ����WebSocket��Ϣ�����¼�
void WebSocketServer::handleMessage(uWS::WebSocket<false, true, PerSocketData>* ws, std::string_view message, uWS::OpCode opCode) {
    PerSocketData* userData = (PerSocketData*)ws->getUserData(); // ��ȡ���ӵ��û�����
    std::cout << "Message received from client. Connection time: " << userData->connectionTime
        << ", IP address: " << userData->ipAddress << std::endl; // ������յ�����Ϣ��Ϣ
    ws->send(message, opCode); // �����յ�����Ϣԭ�����ͻؿͻ���
}

// ����WebSocket���ӹر��¼�
void WebSocketServer::handleClose(uWS::WebSocket<false, true, PerSocketData>* ws, int code, std::string_view message) {
    PerSocketData* userData = (PerSocketData*)ws->getUserData(); // ��ȡ���ӵ��û�����
    std::cout << "WebSocket connection closed with code: " << code
        << " and message: " << message
        << ". Connection time: " << userData->connectionTime
        << ", IP address: " << userData->ipAddress << std::endl; // ������ӹرյ���Ϣ
}

// ����WebSocket������
void WebSocketServer::run() {
    app.listen(9001, [](auto* listen_socket) {
        if (listen_socket) {
            std::cout << "Listening on port " << 9001 << std::endl; // �ɹ������˿�9001
        }
        else {
            std::cout << "Failed to listen on port 9001" << std::endl; // �����˿�9001ʧ��
        }
    }).run(); // ����������
}