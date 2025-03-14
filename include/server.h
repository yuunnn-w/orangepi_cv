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

            // ������ accept ����
            new_socket = accept(m_server_fd, (struct sockaddr*)&client_addr, &addrlen);

            if (new_socket < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // û���µ����ӣ��������ߺ����ѭ��
                    usleep(10000); // ����10���룬��ֹCPU����ռ��
                    continue;
                }
                else {
                    // ����������
                    // ������...
                    usleep(10000); // ���������Ҳ��΢�ȴ�һ��
                    continue;
                }
            }

            // Handle client in new thread
            std::thread client_thread(&TCPServer::handle_client, this, new_socket);
            m_worker_threads.emplace_back(std::move(client_thread));
        }

        // �رշ������׽���
        close(m_server_fd);

        // �ȴ����й����߳����
        for (auto& t : m_worker_threads) {
            if (t.joinable()) {
                t.join();
            }
        }
    }

    void handle_client(int client_fd) {
        // Read command
        ssize_t read_result;
        uint32_t cmd_len;
        // ��ȡ�����
        if ((read_result = read(client_fd, &cmd_len, sizeof(cmd_len))) == -1) {
            perror("Error reading command length");
            close(client_fd); // ȷ���رտͻ�������
            return;
        }
        if (read_result == 0) { // �ͻ��˹ر�����
            close(client_fd);
            return;
        }
        cmd_len = ntohl(cmd_len);

        if (cmd_len > 255) { // ��ֹ���������
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
        if (read_result < cmd_len) { // �����ֶ�ȡ�����
            perror("Incomplete command read");
            close(client_fd);
            return;
        }
        cmd[cmd_len] = '\0';

        if (strcmp(cmd, "video") == 0) {
            std::thread(&TCPServer::handle_video, this, client_fd).detach();
        }
        else if (strcmp(cmd, "control") == 0) {
            std::thread(&TCPServer::handle_control, this, client_fd).detach();
        }
        else {
            // ����δ֪��������
            perror("Unknown command");
        }
    }

    void handle_video(int client_fd) {
        const int target_fps = 30; // Ŀ��֡�ʣ��ɸ�����Ҫ����
        const std::chrono::microseconds frame_interval(1000000 / target_fps);

        while (m_running) {
            auto start_time = std::chrono::steady_clock::now();

            try {
                // ��ȡ���µ�������
                auto inference_result = image_queue.get_latest_inference();
                cv::Mat frame = std::get<cv::Mat>(inference_result);
                auto od_results = std::get<object_detect_result_list>(inference_result);

                // ���Ƽ���
                cv::Mat output_frame = draw_detection_boxes(frame, od_results);

                // JPEG����
                std::vector<uint8_t> buffer;
                cv::imencode(".jpg", output_frame, buffer, { cv::IMWRITE_JPEG_QUALITY, 90 });

                // ����֡����ͷ
                uint32_t frame_size = htonl(static_cast<uint32_t>(buffer.size()));
                if (send(client_fd, &frame_size, sizeof(frame_size), 0) < 0) {
                    perror("send frame size");
                    break;
                }

                // ����֡����
                if (send(client_fd, buffer.data(), buffer.size(), 0) < 0) {
                    perror("send frame data");
                    break;
                }
            }
            catch (const std::exception& e) {
                std::cerr << "Error sending video frame: " << e.what() << std::endl;
                break;
            }

            // ����ʣ��ʱ�䲢����
            auto end_time = std::chrono::steady_clock::now();
            auto processing_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
            auto sleep_time = frame_interval - processing_time;

            if (sleep_time > std::chrono::microseconds(0)) {
                std::this_thread::sleep_for(sleep_time);
                // ��ӡ��˯���˶��
				//std::cout << "Slept for " << sleep_time.count() << " microseconds" << std::endl;
            }
            else {
                // ����ʱ��������棨��ѡ��
                //std::cerr << "Warning: Frame processing too slow to maintain target FPS\n";
            }
        }

        close(client_fd);
        std::cout << "Connection closed" << std::endl;
    }

    void handle_control(int client_fd) {
        ssize_t read_result;
        uint32_t ctrl_len;
        // ��ȡ���������
        if ((read_result = read(client_fd, &ctrl_len, sizeof(ctrl_len))) == -1) {
            perror("Error reading control command length");
            close(client_fd);
            return;
        }
        if (read_result == 0) { // �ͻ��˹ر�����
            close(client_fd);
            return;
        }
        ctrl_len = ntohl(ctrl_len);
        if (ctrl_len > 255) { // ��ֹ���������
            perror("Control command length too long");
            close(client_fd);
            return;
        }
        char ctrl_cmd[256];
        if ((read_result = read(client_fd, ctrl_cmd, ctrl_len)) == -1) {
            perror("Error reading control command");
            close(client_fd);
            return;
        }
        if (read_result < ctrl_len) { // �����ֶ�ȡ�����
            perror("Incomplete control command read");
            close(client_fd);
            return;
        }
        ctrl_cmd[ctrl_len] = '\0';

        if (strcmp(ctrl_cmd, "getframenow") == 0) {
            // ��ȡ�����е�֡
            std::tuple<uint64_t, cv::Mat, std::string, object_detect_result_list, std::vector<float>> inference_result;
            inference_result = image_queue.get_latest_inference();
            cv::Mat frame = std::get<cv::Mat>(inference_result); // ��ȡͼ��
            object_detect_result_list od_results = std::get<object_detect_result_list>(inference_result); // ��ȡ�����
            // ���û��ƺ������Ƽ���
            cv::Mat output_frame = draw_detection_boxes(frame, od_results);
            // ʹ�� JPEG ����
            std::vector<uint8_t> buffer;
            cv::imencode(".webp", output_frame, buffer, { cv::IMWRITE_WEBP_QUALITY, 90 });
            // �������ֽڵĳ���ͷ
            uint32_t frame_size = static_cast<uint32_t>(buffer.size());
            frame_size = htonl(frame_size); // ת��Ϊ�����ֽ���
            if (send(client_fd, &frame_size, sizeof(frame_size), 0) < 0) {
                perror("send frame size");
                close(client_fd);
                return;
            }
            // ����֡����
            if (send(client_fd, buffer.data(), buffer.size(), 0) < 0) {
                perror("send frame data");
                close(client_fd);
                return;
            }
        }
        else if (strcmp(ctrl_cmd, "sleep") == 0) {
			system_sleep = true;
            std::cout << "system_sleep set to true." << std::endl;
            // Send response
            const char* resp = "Sleep Success";
            uint32_t len = htonl(strlen(resp));
            if (send(client_fd, &len, sizeof(len), 0) < 0) {
                perror("send sleep response length");
                close(client_fd);
                return;
            }
            if (send(client_fd, resp, strlen(resp), 0) < 0) {
                perror("send sleep response");
                close(client_fd);
                return;
            }
        }
        else if (strcmp(ctrl_cmd, "poweroff") == 0) {
            poweroff = true;
            std::cout << "poweroff set to true." << std::endl;
            // Send response then shutdown
            const char* resp = "Poweroff Success";
            uint32_t len = htonl(strlen(resp));
            if (send(client_fd, &len, sizeof(len), 0) < 0) {
                perror("send poweroff response length");
                close(client_fd);
                return;
            }
            if (send(client_fd, resp, strlen(resp), 0) < 0) {
                perror("send poweroff response");
                close(client_fd);
                return;
            }
            // stop(); // �����Ҫ�ڴ˴�ֹͣ������ȡ��ע�ʹ���
        }
        else if (strcmp(ctrl_cmd, "wakeup") == 0) {
            system_sleep = false;
            std::cout << "system_sleep set to false." << std::endl;
            // Send response then shutdown
            const char* resp = "Wakeup Success";
            uint32_t len = htonl(strlen(resp));
            if (send(client_fd, &len, sizeof(len), 0) < 0) {
                perror("send wakeup response length");
                close(client_fd);
                return;
            }
            if (send(client_fd, resp, strlen(resp), 0) < 0) {
                perror("send wakeup response");
                close(client_fd);
                return;
            }
            // stop(); // �����Ҫ�ڴ˴�ֹͣ������ȡ��ע�ʹ���
        }
        close(client_fd);
    }
};

#endif // SERVER_H