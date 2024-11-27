#include "App.h"
#include <iostream>
#include <string>
#include <chrono>
#include <ctime>

int SSS() {
    /* ws->getUserData returns one of these */
    struct PerSocketData {
        /* Fill with user data */
        std::string connectionTime;  // 连接建立时间
        std::string ipAddress;       // 用户IP地址
    };

    /* Create an instance of uWS::App without SSL */
    uWS::App app = uWS::App();

    /* Define WebSocket handlers */
    app.ws<PerSocketData>("/*", {
        /* Settings */
        .compression = uWS::SHARED_COMPRESSOR,
        .maxPayloadLength = 16 * 1024,
        .idleTimeout = 10,
        .maxBackpressure = 1 * 1024 * 1024,
        .closeOnBackpressureLimit = false,
        .resetIdleTimeoutOnSend = true,
        .sendPingsAutomatically = true,
        /* Handlers */
        .open = [](auto* ws) {
            // 获取当前时间
            auto now = std::chrono::system_clock::now();
            std::time_t now_time = std::chrono::system_clock::to_time_t(now);
            std::tm* now_tm = std::localtime(&now_time);
            char time_str[20];
            std::strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", now_tm);
            // 获取用户的 IP 地址
            std::string ipAddress = std::string(ws->getRemoteAddressAsText());
            // 初始化用户数据
            PerSocketData* userData = ws->getUserData();
            userData->connectionTime = time_str;
            userData->ipAddress = ipAddress;

            ws->send("Hello, World!", uWS::OpCode::TEXT);
        },
        .message = [](auto* ws, std::string_view message, uWS::OpCode opCode) {
            PerSocketData* userData = (PerSocketData*)ws->getUserData();
            std::cout << "Message received from client. Connection time: " << userData->connectionTime
                << ", IP address: " << userData->ipAddress << std::endl;
            /* Echo back the received message */
            ws->send(message, opCode);
        },
        .close = [](auto* ws, int code, std::string_view message) {
            PerSocketData* userData = (PerSocketData*)ws->getUserData();
            std::cout << "WebSocket connection closed with code: " << code
                << " and message: " << message
                << ". Connection time: " << userData->connectionTime
                << ", IP address: " << userData->ipAddress << std::endl;
        }
        }).ws<PerSocketData>("/help", {
            /* Settings */
            .compression = uWS::SHARED_COMPRESSOR,
            .maxPayloadLength = 16 * 1024,
            .idleTimeout = 10,
            .maxBackpressure = 1 * 1024 * 1024,
            .closeOnBackpressureLimit = false,
            .resetIdleTimeoutOnSend = true,
            .sendPingsAutomatically = true,
            /* Handlers */
            .open = [](auto* ws) {
                PerSocketData* userData = (PerSocketData*)ws->getUserData();
                /* Open event here, send help message to the client */
                ws->send("This is the help route. How can I assist you?", uWS::OpCode::TEXT);
            },
            .message = [](auto* ws, std::string_view message, uWS::OpCode opCode) {
                PerSocketData* userData = (PerSocketData*)ws->getUserData();
                /* Echo back the received message */
                ws->send(message, opCode);
            },
            .close = [](auto* ws, int code, std::string_view message) {
                PerSocketData* userData = (PerSocketData*)ws->getUserData();
                /* Close event here */
                std::cout << "WebSocket connection closed with code: " << code << " and message: " << message << std::endl;
            }
            }).ws<PerSocketData>("/tool", {
                /* Settings */
                .compression = uWS::SHARED_COMPRESSOR,
                .maxPayloadLength = 16 * 1024,
                .idleTimeout = 10,
                .maxBackpressure = 1 * 1024 * 1024,
                .closeOnBackpressureLimit = false,
                .resetIdleTimeoutOnSend = true,
                .sendPingsAutomatically = true,
                /* Handlers */
                .open = [](auto* ws) {
                    PerSocketData* userData = (PerSocketData*)ws->getUserData();
                    /* Open event here, send tool message to the client */
                    ws->send("This is the tool route. What tool do you need?", uWS::OpCode::TEXT);
                },
                .message = [](auto* ws, std::string_view message, uWS::OpCode opCode) {
                    PerSocketData* userData = (PerSocketData*)ws->getUserData();
                    /* Echo back the received message */
                    ws->send(message, opCode);
                },
                .close = [](auto* ws, int code, std::string_view message) {
                    PerSocketData* userData = (PerSocketData*)ws->getUserData();
                    /* Close event here */
                    std::cout << "WebSocket connection closed with code: " << code << " and message: " << message << std::endl;
                }
                }).listen(9001, [](auto* listen_socket) {
                    if (listen_socket) {
                        std::cout << "Listening on port " << 9001 << std::endl;
                    }
                    else {
                        std::cout << "Failed to listen on port 9001" << std::endl;
                    }
                    }).run();

    return 0;
}