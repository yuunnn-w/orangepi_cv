// Server.h
#ifndef SERVER_H
#define SERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>
#include <atomic>
#include <vector>
#include <cstring>
#include <string>
#include <opencv2/opencv.hpp>
#include <arpa/inet.h> // For ntohl
#include <fcntl.h>
#include <errno.h>
#include <nlohmann/json.hpp>

#include "threadsafequeue.h"
#include "utils.h"



cv::Mat draw_detection_boxes(const cv::Mat& image, const object_detect_result_list& od_results);

class TCPServer {
public:
    TCPServer() : m_running(false), m_server_fd(-1) {}

    // ~TCPServer() {}

    void run(int port) {
        m_running = true;
        m_listener_thread = std::thread(&TCPServer::listener_loop, this, port);
    }

    void stop() {
        m_running = false;
        if (m_server_fd != -1) {
            shutdown(m_server_fd, SHUT_RDWR);
            close(m_server_fd);
        }
        if (m_listener_thread.joinable()) {
            m_listener_thread.join();
        }
        // Join all worker threads...
    }

private:
    std::atomic<bool> m_running;
    int m_server_fd;
    std::thread m_listener_thread;
    std::vector<std::thread> m_worker_threads;

    void listener_loop(int port) {
        struct sockaddr_in address;
        const int opt = 1;

        // Create socket
        if ((m_server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
            // Error handling
            return;
        }

        // Set socket options and make it non-blocking
        setsockopt(m_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        int flags = fcntl(m_server_fd, F_GETFL, 0);
        fcntl(m_server_fd, F_SETFL, flags | O_NONBLOCK);

        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);

        // Bind
        if (bind(m_server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            // Error handling
            close(m_server_fd);
            return;
        }

        // Listen
        if (listen(m_server_fd, 5) < 0) {
            // Error handling
            close(m_server_fd);
            return;
        }

        // Accept loop
        while (m_running) {
            int new_socket;
            sockaddr_in client_addr;
            socklen_t addrlen = sizeof(client_addr);

            // 非阻塞 accept 调用
            new_socket = accept(m_server_fd, (struct sockaddr*)&client_addr, &addrlen);

            if (new_socket < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // 没有新的连接，短暂休眠后继续循环
                    usleep(10000); // 休眠10毫秒，防止CPU过度占用
                    continue;
                }
                else {
                    // 其他错误发生
                    // 错误处理...
                    usleep(10000); // 发生错误后也稍微等待一下
                    continue;
                }
            }

            // Handle client in new thread
            std::thread client_thread(&TCPServer::handle_client, this, new_socket);
            m_worker_threads.emplace_back(std::move(client_thread));
        }

        // 关闭服务器套接字
        close(m_server_fd);

        // 等待所有工作线程完成
        for (auto& t : m_worker_threads) {
            if (t.joinable()) {
                t.join();
            }
        }
    }

    void handle_client(int client_fd) {
        ssize_t read_result;
        uint32_t cmd_len;

        // 读取命令长度
        if ((read_result = read(client_fd, &cmd_len, sizeof(cmd_len))) == -1) {
            perror("Error reading command length");
            close(client_fd);
            return;
        }
        if (read_result == 0) {
            close(client_fd);
            return;
        }
        cmd_len = ntohl(cmd_len);

        if (cmd_len > 255) {
            perror("Command length too long");
            close(client_fd);
            return;
        }

        char cmd[256];
        if ((read_result = read(client_fd, cmd, cmd_len)) == -1) {
            perror("Error reading command");
            close(client_fd);
            return;
        }
        if (read_result < cmd_len) {
            perror("Incomplete command read");
            close(client_fd);
            return;
        }
        cmd[cmd_len] = '\0';

        try {
            // 解析JSON命令
            json json_cmd = json::parse(cmd);

            // 验证必须包含command字段
            if (!json_cmd.contains("command") || !json_cmd["command"].is_string()) {
                throw std::runtime_error("Missing or invalid 'command' field");
            }

            std::string command = json_cmd["command"].get<std::string>();

            // 处理不同命令
            if (command == "video") {
                std::thread(&TCPServer::handle_video, this, client_fd).detach();
            }
            else if (command == "control") {
                std::thread(&TCPServer::handle_control, this, client_fd).detach();
                // 阻止舵机控制

            }
            // 控制舵机
			else if (command == "servo") {
                std::thread(&TCPServer::servo_control, this, client_fd).detach();
			}
			else if (command == "exit") {
				close(client_fd);
				return;

			}
            else {
                std::cerr << "Unknown command: " << command << std::endl;
                close(client_fd);
            }

        }
        catch (const json::exception& e) {
            std::cerr << "JSON parse error: " << e.what() << "\nReceived: " << cmd << std::endl;
            close(client_fd);
        }
        catch (const std::exception& e) {
            std::cerr << "Command error: " << e.what() << "\nReceived: " << cmd << std::endl;
            close(client_fd);
		}
        catch (...) {
            std::cerr << "Unknown error occurred while processing command: " << cmd << std::endl;
            close(client_fd);
        }
    }

	// 控制舵机
    void servo_control(int client_fd) {
		// 禁止自主控制
		allow_auto_control = false;
        while (m_running) {
            try {
                float angles[2] = { 0 };
                // 直接读取8字节（两个float）
                ssize_t total_read = 0;
                char* buffer = reinterpret_cast<char*>(angles);

                while (total_read < sizeof(angles)) {
                    ssize_t bytes_read = read(client_fd,
                        buffer + total_read,
                        sizeof(angles) - total_read);

                    if (bytes_read <= 0) {
						std::cerr << " Error reading angles: " << strerror(errno) << std::endl;
                        close(client_fd);
						allow_auto_control = true;
                        return ;
                    }
                    total_read += bytes_read;
                }

                // 直接使用内存中的数据（无需字节序转换）
                // 设置舵机角度
                // 舵机互斥锁保护
                std::unique_lock<std::mutex> servo_lock(servo_mtx);
                std::vector<uint8_t> ids = { 1, 2 };
                // 根据全局舵机角度变量和接收到的偏移量计算新的舵机角度
                // 这里 angles[0] 和 angles[1] 是舵机1和舵机2的角度偏移量
                // 计算新的舵机角度
                std::vector<float> angles_vec = {
                    ServoXAngle + angles[0], // 舵机1的角度
                    ServoYAngle + angles[1]  // 舵机2的角度
                };
				// 如果超出范围，则限制在范围内
                // 限制舵机角度在范围内
                angles_vec[0] = std::clamp(angles_vec[0], SERVO1_MIN_ANGLE, SERVO1_MAX_ANGLE); // 限制舵机1的角度
                angles_vec[1] = std::clamp(angles_vec[1], SERVO2_MIN_ANGLE, SERVO2_MAX_ANGLE); // 限制舵机2的角度
				
                // 同步控制舵机
                servoDriver.syncSetTargetPositions(ids, angles_vec);
                usleep(15000); // 等待15毫秒
                servo_lock.unlock();
            }
			catch (const std::exception& e) {
				std::cerr << "Error in servo control: " << e.what() << std::endl;
				break;
			}
			catch (...) {
				std::cerr << "Unknown error occurred in servo control" << std::endl;
				break;
			}
		}
		// 允许自主控制
		allow_auto_control = true;
		close(client_fd);
        std::cout << "Servo control connection closed" << std::endl;
    };



    void handle_video(int client_fd) {
        const int target_fps = 30; // 目标帧率，可根据需要调整
        const std::chrono::microseconds frame_interval(1000000 / target_fps);

        while (m_running) {
            auto start_time = std::chrono::steady_clock::now();

            try {
                // 获取最新的推理结果
                auto inference_result = image_queue.get_latest_inference();
                cv::Mat frame = std::get<cv::Mat>(inference_result);
                auto od_results = std::get<object_detect_result_list>(inference_result);

                // 绘制检测框
                cv::Mat output_frame = draw_detection_boxes(frame, od_results);

                // JPEG编码
                std::vector<uint8_t> buffer;
                cv::imencode(".jpg", output_frame, buffer, { cv::IMWRITE_JPEG_QUALITY, 90 });
				// WEBP编码，帧率很低而且占用CPU较高
				// cv::imencode(".webp", output_frame, buffer, { cv::IMWRITE_WEBP_QUALITY, 90 }); // 0-100，大于100表示无损压缩
                // cv::imencode(".png", output_frame, buffer, { cv::IMWRITE_PNG_COMPRESSION, 1 });
                // 发送帧长度头
                uint32_t frame_size = htonl(static_cast<uint32_t>(buffer.size()));
                if (send(client_fd, &frame_size, sizeof(frame_size), 0) < 0) {
                    perror("send frame size");
                    break;
                }

                // 发送帧数据
                if (send(client_fd, buffer.data(), buffer.size(), 0) < 0) {
                    perror("send frame data");
                    break;
                }
            }
            catch (const std::exception& e) {
                std::cerr << "Error sending video frame: " << e.what() << std::endl;
                break;
            }

            // 计算剩余时间并休眠
            auto end_time = std::chrono::steady_clock::now();
            auto processing_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
            auto sleep_time = frame_interval - processing_time;

            if (sleep_time > std::chrono::microseconds(0)) {
                std::this_thread::sleep_for(sleep_time);
                // 打印出睡眠了多久
				//std::cout << "Slept for " << sleep_time.count() << " microseconds" << std::endl;
            }
            else {
                // 处理超时，输出警告
                //std::cerr << "Warning: Frame processing too slow to maintain target FPS\n";
            }
        }

        close(client_fd);
        std::cout << "Connection closed" << std::endl;
    }

    void handle_control(int client_fd) {
        try {
            // 第二阶段：接收操作指令
            uint32_t phase2_len;
            if (read(client_fd, &phase2_len, sizeof(phase2_len)) != sizeof(phase2_len)) {
                throw std::runtime_error("Failed to read phase2 length");
                // 关闭连接
                close(client_fd);
                return;
            }
            phase2_len = ntohl(phase2_len);

            if (phase2_len > 1024 * 1024) { // 限制1MB负载
                throw std::runtime_error("Phase2 payload too large");
                // 关闭连接
                close(client_fd);
                return;
            }

            std::vector<char> phase2_buf(phase2_len + 1);
            ssize_t received = 0;
            while (received < phase2_len) {
                ssize_t n = read(client_fd, phase2_buf.data() + received, phase2_len - received);
                if (n <= 0) throw std::runtime_error("Phase2 read error");
                received += n;
            }
            phase2_buf[phase2_len] = '\0';

            // 解析操作指令
            json phase2 = json::parse(phase2_buf.data());
            std::string action = phase2["action"];
            json params = phase2["params"];

            // 处理命令
            json response;
            response["code"] = 200;
            response["timestamp"] = time(nullptr);

            if (action == "getframenow") {
                // 获取最新帧数据
                std::tuple<uint64_t, cv::Mat, std::string, object_detect_result_list, std::vector<float>> inference_result;
                inference_result = image_queue.get_latest_inference();
                cv::Mat frame = std::get<cv::Mat>(inference_result);
                // object_detect_result_list od_results = std::get<object_detect_result_list>(inference_result); // 获取检测结果
                // 调用绘制函数绘制检测框
                // cv::Mat output_frame = draw_detection_boxes(frame, od_results);
                // 使用 WEBP 编码
                std::vector<uint8_t> buffer;
                cv::imencode(".png", frame, buffer, { cv::IMWRITE_PNG_COMPRESSION, 1 });// 直接用png格式，无损格式
                //cv::imencode(".webp", frame, buffer, { cv::IMWRITE_WEBP_QUALITY, 101 }); // 大于100表示无损压缩

                // 构造响应
                response["content"] = buffer;
            }
            else if (action == "sleep") {
                system_sleep = true;
                response["content"] = "Enter sleep mode";
            }
            else if (action == "wakeup") {
                system_sleep = false;
                response["content"] = "System wakeup";
            }
            else if (action == "poweroff") {
                poweroff = true;
                response["content"] = "System shutting down";
            }
            else if (action == "get_status") {
                // 根据系统状态设置状态描述
                std::string status = system_sleep ? "sleeping" : "running";
                // 包含舵机位置和目标位置
				response["content"]["ServoXAngle"] = std::to_string(ServoXAngle).c_str();
				response["content"]["ServoYAngle"] = std::to_string(ServoYAngle).c_str();
				response["content"]["TargetXAngle"] = std::to_string(targetX).c_str();
				response["content"]["TargetYAngle"] = std::to_string(targetY).c_str();
                // 包含自己的位置
				response["content"]["SelfX"] = std::to_string(SELF_X).c_str();
				response["content"]["SelfY"] = std::to_string(SELF_Y).c_str();

                // 系统状态
				response["content"]["system_status"] = status;
				// 摄像头状态，处于可见光还是红外
				response["content"]["camera_mode"] = camera_mode ? "infrared" : "visible";
                // 是否识别到目标
				response["content"]["target_detected"] = target_detected ? "yes" : "no";
                // std::cout << "System status sent: " << status << std::endl;
            }
            else {
                response["code"] = 404;
                response["content"] = "Unsupported action";
            }

            // 发送响应
            std::string json_str = response.dump();
            uint32_t resp_len = htonl(json_str.size());

            if (send(client_fd, &resp_len, sizeof(resp_len), 0) != sizeof(resp_len)) {
                throw std::runtime_error("Send response length failed");
                close(client_fd);
                return;
            }

            if (send(client_fd, json_str.data(), json_str.size(), 0) != (ssize_t)json_str.size()) {
                throw std::runtime_error("Send response failed");
                close(client_fd);
                return;
            }
        }
        catch (const json::exception& e) {
            std::cerr << "JSON parse error: " << e.what() << std::endl;
            // 关闭连接
            close(client_fd);
            return;
        }
        catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return;
        }
        catch (...) {
            std::cerr << "Unknown error occurred" << std::endl;
            return;
        }
        close(client_fd);
        return;
    }

};

#endif // SERVER_H