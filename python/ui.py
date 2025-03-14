import os
import cv2
import sys
import socket
import struct
import threading
import numpy as np
from datetime import datetime
from PyQt5.QtCore import Qt, pyqtSignal, QObject, QThread
from PyQt5.QtGui import QPixmap, QImage
from PyQt5.QtWidgets import (QApplication, QMainWindow, QWidget, QSplitter, QLabel,
                             QLineEdit, QPushButton, QTextEdit, QVBoxLayout, QHBoxLayout,
                             QGridLayout, QGroupBox, QMessageBox, QSizePolicy)


class VideoReceiver(QThread):
    frame_received = pyqtSignal(bytes)
    error_occurred = pyqtSignal(str)
    
    def __init__(self, host, port):
        super().__init__()
        self.host = host
        self.port = port
        self.running = False
        self.connection = None

    def run(self):
        try:
            self.connection = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.connection.connect((self.host, self.port))
            
            # Send video command
            cmd = b"video"
            self.connection.send(struct.pack("!I", len(cmd)) + cmd)
            
            self.running = True
            while self.running:
                # Read frame length (4 bytes)
                raw_len = self.connection.recv(4)
                if not raw_len:
                    break
                frame_len = struct.unpack("!I", raw_len)[0]
                
                # Read frame data
                frame_data = bytearray()
                while len(frame_data) < frame_len:
                    packet = self.connection.recv(frame_len - len(frame_data))
                    if not packet:
                        break
                    frame_data.extend(packet)
                
                if len(frame_data) == frame_len:
                    self.frame_received.emit(bytes(frame_data))
                    
        except Exception as e:
            self.error_occurred.emit(str(e))
        finally:
            if self.connection:
                self.connection.close()

    def stop(self):
        self.running = False

class ControlClient(QObject):
    response_received = pyqtSignal(str)
    frame_received = pyqtSignal(bytes)
    error_occurred = pyqtSignal(str)

    def send_command(self, host, port, command):
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.connect((host, port))
                
                # Send control command
                cmd = b"control"
                s.send(struct.pack("!I", len(cmd)) + cmd)
                
                # Send sub-command
                s.send(struct.pack("!I", len(command)) + command.encode())
                
                # Handle response
                if command == "getframenow":
                    raw_len = s.recv(4)
                    if not raw_len:
                        return
                    frame_len = struct.unpack("!I", raw_len)[0]
                    
                    frame_data = bytearray()
                    while len(frame_data) < frame_len:
                        packet = s.recv(frame_len - len(frame_data))
                        if not packet:
                            break
                        frame_data.extend(packet)
                    
                    if len(frame_data) == frame_len:
                        self.frame_received.emit(bytes(frame_data))
                else:
                    raw_len = s.recv(4)
                    if not raw_len:
                        return
                    resp_len = struct.unpack("!I", raw_len)[0]
                    
                    response = bytearray()
                    while len(response) < resp_len:
                        packet = s.recv(resp_len - len(response))
                        if not packet:
                            break
                        response.extend(packet)
                    
                    self.response_received.emit(response.decode())

        except Exception as e:
            self.error_occurred.emit(str(e))

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.video_thread = None
        self.control_client = None
        self.init_ui()
        
    def init_ui(self):
        self.setWindowTitle("视频监控客户端")
        self.setGeometry(100, 100, 1200, 900)  # 更大的初始窗口尺寸
    
        # 主分割器（左右比例 1:4）
        main_splitter = QSplitter(Qt.Horizontal)
        
        # 左侧控制面板（使用垂直分割器实现可调节区域）
        left_splitter = QSplitter(Qt.Vertical)
        
        # 服务器设置区域
        server_group = QGroupBox("服务器设置")
        server_layout = QVBoxLayout()
        self.server_input = QLineEdit("172.21.49.5:8080")
        self.server_input.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)
        server_layout.addWidget(QLabel("服务器地址:"))
        server_layout.addWidget(self.server_input)
        server_group.setLayout(server_layout)
        
        # 视频控制区域
        video_group = QGroupBox("视频控制")
        video_layout = QHBoxLayout()
        self.start_btn = QPushButton("开始视频")
        self.stop_btn = QPushButton("停止视频")
        self.stop_btn.setEnabled(False)
        video_layout.addWidget(self.start_btn, stretch=1)
        video_layout.addWidget(self.stop_btn, stretch=1)
        video_group.setLayout(video_layout)
        
        # 系统控制区域
        control_group = QGroupBox("系统控制")
        control_layout = QGridLayout()
        self.frame_btn = QPushButton("获取当前帧")
        self.sleep_btn = QPushButton("系统休眠")
        self.wakeup_btn = QPushButton("系统唤醒")
        self.power_btn = QPushButton("系统关机")
        control_layout.addWidget(self.frame_btn, 0, 0, 1, 2)
        control_layout.addWidget(self.sleep_btn, 1, 0)
        control_layout.addWidget(self.wakeup_btn, 1, 1)
        control_layout.addWidget(self.power_btn, 2, 0, 1, 2)
        control_group.setLayout(control_layout)
    
        # 组合左侧布局（各部分比例 2:1:3）
        left_splitter.addWidget(server_group)
        left_splitter.addWidget(video_group)
        left_splitter.addWidget(control_group)
        left_splitter.setSizes([150, 100, 250])  # 初始高度分配
        left_splitter.setCollapsible(0, False)   # 禁止折叠
        
        # 右侧显示区域（上下比例 3:1）
        right_splitter = QSplitter(Qt.Vertical)
        self.video_label = QLabel()
        self.video_label.setAlignment(Qt.AlignCenter)
        self.video_label.setStyleSheet("background-color: black;")
        self.video_label.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        
        self.log_output = QTextEdit()
        self.log_output.setReadOnly(True)
        self.log_output.setMinimumHeight(150)  # 最小高度限制
        
        right_splitter.addWidget(self.video_label)
        right_splitter.addWidget(self.log_output)
        right_splitter.setSizes([600, 200])  # 初始比例 3:1
        
        # 组合主界面（左右比例 1:4）
        main_splitter.addWidget(left_splitter)
        main_splitter.addWidget(right_splitter)
        main_splitter.setSizes([240, 960])  # 初始宽度分配
        main_splitter.setStretchFactor(0, 1)
        main_splitter.setStretchFactor(1, 4)
        
        self.setCentralWidget(main_splitter)
    
        # 连接信号（保持不变）
        self.start_btn.clicked.connect(self.start_video)
        self.stop_btn.clicked.connect(self.stop_video)
        self.frame_btn.clicked.connect(lambda: self.send_control("getframenow"))
        self.sleep_btn.clicked.connect(lambda: self.send_control("sleep"))
        self.wakeup_btn.clicked.connect(lambda: self.send_control("wakeup"))
        self.power_btn.clicked.connect(lambda: self.send_control("poweroff"))
        
    def log_message(self, message):
        timestamp = datetime.now().strftime("%H:%M:%S")
        self.log_output.append(f"[{timestamp}] {message}")
        self.log_output.verticalScrollBar().setValue(
            self.log_output.verticalScrollBar().maximum())
        
    def start_video(self):
        try:
            host, port = self.server_input.text().split(":")
            port = int(port)
        except:
            self.log_message("服务器地址格式错误")
            return
            
        if self.video_thread and self.video_thread.isRunning():
            return
            
        self.video_thread = VideoReceiver(host, port)
        self.video_thread.frame_received.connect(self.show_video_frame)
        self.video_thread.error_occurred.connect(self.log_message)
        self.video_thread.start()
        
        self.start_btn.setEnabled(False)
        self.stop_btn.setEnabled(True)
        self.log_message("视频流连接已建立")
        
    def stop_video(self):
        if self.video_thread:
            self.video_thread.stop()
            self.video_thread.quit()
            self.video_thread.wait()
            
        self.start_btn.setEnabled(True)
        self.stop_btn.setEnabled(False)
        self.log_message("视频接收已停止")

    def show_video_frame(self, frame_data):
        try:
            # 使用OpenCV处理颜色转换
            nparr = np.frombuffer(frame_data, np.uint8)
            img = cv2.imdecode(nparr, cv2.IMREAD_COLOR)
            if img is not None:
                # BGR转RGB
                # img = cv2.cvtColor(img, cv2.COLOR_RGB2BGR)
                h, w, ch = img.shape
                bytes_per_line = ch * w
                qimg = QImage(img.data, w, h, bytes_per_line, QImage.Format_RGB888)
                pixmap = QPixmap.fromImage(qimg)
                self.video_label.setPixmap(
                    pixmap.scaled(self.video_label.size(), 
                                 Qt.KeepAspectRatio,
                                 Qt.SmoothTransformation))
        except Exception as e:
            self.log_message(f"图像处理错误: {str(e)}")
            
    def send_control(self, command):
        try:
            host, port = self.server_input.text().split(":")
            port = int(port)
        except:
            self.log_message("服务器地址格式错误")
            return
            
        self.control_client = ControlClient()
        self.control_client.response_received.connect(self.log_message)
        self.control_client.frame_received.connect(self.save_frame)
        self.control_client.error_occurred.connect(self.log_message)
        
        thread = threading.Thread(
            target=self.control_client.send_command,
            args=(host, port, command)
        )
        thread.start()
        
        self.log_message(f"已发送控制指令: {command}")
        
    def save_frame(self, frame_data):
        try:
            # 解码图像数据
            nparr = np.frombuffer(frame_data, np.uint8)
            img = cv2.imdecode(nparr, cv2.IMREAD_COLOR)
            img = cv2.cvtColor(img, cv2.COLOR_RGB2BGR)
            
            if img is None:
                raise ValueError("解码失败：无效的图像数据")
    
            # 创建存储目录
            save_dir = "frames"
            os.makedirs(save_dir, exist_ok=True)
            
            # 生成带时间戳的文件名
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S_%f")[:-3]  # 毫秒级时间戳
            filename = f"{save_dir}/{timestamp}.png"  # 修改为png扩展名
            
            # 设置PNG压缩参数（0-9，0是无压缩，9是最大压缩）
            compression_params = [cv2.IMWRITE_PNG_COMPRESSION, 0]  # 中等压缩级别
            
            if not cv2.imwrite(filename, img, compression_params):
                raise RuntimeError("PNG图像写入失败")
                
            # 显示绝对路径
            abs_path = os.path.abspath(filename)
            QMessageBox.information(
                self, 
                "保存成功", 
                f"当前帧已保存至：\n{abs_path}",
                QMessageBox.Ok
            )
            
        except Exception as e:
            error_msg = f"保存PNG帧失败: {str(e)}"
            self.log_message(error_msg)
            QMessageBox.critical(
                self,
                "保存失败",
                error_msg,
                QMessageBox.Ok
            )


if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    sys.exit(app.exec_())