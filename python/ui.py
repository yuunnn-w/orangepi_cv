import json
import math
import os
import cv2
import sys
import time
import socket
import struct
import threading
import numpy as np
from pynput import keyboard
from datetime import datetime
from PyQt5.QtCore import Qt, pyqtSignal, QObject, QThread, QTimer
from PyQt5.QtGui import QPixmap, QImage
from PyQt5.QtWidgets import (QApplication, QMainWindow, QWidget, QSplitter, QLabel,
                             QComboBox, QPushButton, QTextEdit, QVBoxLayout, QHBoxLayout,
                             QGridLayout, QGroupBox, QMessageBox, QSizePolicy, QRadioButton,
                             QLineEdit)

from matplotlib import pyplot as plt
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure
from matplotlib.lines import Line2D

# 设置全局字体为 SimHei（黑体）
plt.rcParams['font.family'] = 'SimHei'
plt.rcParams['axes.unicode_minus'] = False # 解决坐标轴负号显示问题

lock = threading.Lock()

# ================== 全局配置 ==================
# 预定义主机列表
PREDEFINED_HOSTS = {
    "gzy": {
        "ip": "127.0.0.1",
        "system_status": "offline",
        "servo_x": 0.0,
        "servo_y": 0.0,
        "target_x": 0.0,
        "target_y": 0.0,
        "self_x": 0.0,
        "self_y": 0.0,
        "camera_mode": "visible",
        "target_detected": False
    },
    "azi": {
        "ip": "127.0.0.1",
        "system_status": "offline",
        "servo_x": 0.0,
        "servo_y": 0.0,
        "target_x": 0.0,
        "target_y": 0.0,
        "self_x": 5.0,
        "self_y": 5.0,
        "camera_mode": "visible",
        "target_detected": False
    }
}

# 全局目标信息
GLOBAL_TARGET = {
    "detected": False,
    "x": 0.0,
    "y": 0.0,
    "h": 0.0,
    "speed": 0.0
}

BROADCAST_PORT = 5005  # 广播监听端口
CONTROL_PORT = 8080    # 控制端口固定

# 全局变量存储当前选择的主机信息
current_host = {"hostname": "", "ip": "127.0.0.1", "port": CONTROL_PORT}

# 初始化方向键状态字典
keys_pressed = {
    "up": False,
    "down": False,
    "left": False,
    "right": False
}

def on_press(key):
    try:
        char = key.char.lower()
        if char == 'w':
            keys_pressed["up"] = True
        elif char == 's':
            keys_pressed["down"] = True
        elif char == 'a':
            keys_pressed["left"] = True
        elif char == 'd':
            keys_pressed["right"] = True
    except AttributeError:
        pass  # 忽略特殊按键

def on_release(key):
    # 类似on_press，在松开按键时更新状态为False
    try:
        char = key.char.lower()
        if char == 'w':
            keys_pressed["up"] = False
        elif char == 's':
            keys_pressed["down"] = False
        elif char == 'a':
            keys_pressed["left"] = False
        elif char == 'd':
            keys_pressed["right"] = False
    except AttributeError:
        pass

# ================== 渲染窗口 ==================
class RenderWindow(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("三维态势示意图")
        self.setGeometry(200, 200, 800, 600)
        
        # 创建matplotlib画布
        self.figure = Figure(figsize=(8, 6), dpi=100)
        self.canvas = FigureCanvas(self.figure)
        self.ax = self.figure.add_subplot(111, projection='3d')
        
        # 设置布局
        layout = QVBoxLayout()
        layout.addWidget(self.canvas)
        self.setLayout(layout)
        
        # 初始化绘图元素
        self.base_stations = []
        self.target = None
        self.lines = []
        
        # 配置视图参数
        self.ax.set_xlabel('X (m)')
        self.ax.set_ylabel('Y (m)')
        self.ax.set_zlabel('H (m)')
        self.ax.set_title('实时三维态势显示')
        self.ax.grid(True)
        
        # 初始化视图范围
        self.ax.set_xlim(-0.5, 0.5)
        self.ax.set_ylim(-0.5, 0.5)
        self.ax.set_zlim(0, 1)
        
        # 设置定时器（每秒更新20次）
        self.timer = QTimer()
        self.timer.timeout.connect(self.update_plot)
        self.timer.start(50)
        
        # 保存初始视角
        self.initial_view = self.ax.elev, self.ax.azim

    def update_plot(self):
        """更新三维态势图"""
        with lock:
            # 获取最新数据
            stations = [s for s in PREDEFINED_HOSTS.values() 
                       if s["system_status"] != "offline"]
            target = GLOBAL_TARGET.copy()
            
        # 清除旧元素
        for artist in self.base_stations + self.lines:
            artist.remove()
        if self.target:
            self.target.remove()
            
        self.base_stations = []
        self.lines = []
        self.target = None

        try:
            # 绘制基站
            for host in stations:
                x, y = host["self_x"], host["self_y"]
                # 绘制基站立方体
                bs = self.ax.scatter(
                    x, y, 0, 
                    c='blue', 
                    marker='s', 
                    s=100,
                    depthshade=True,
                    edgecolors='black',
                    label='监控基站'
                )
                self.base_stations.append(bs)
                
                # 如果检测到目标，绘制连接线
                if target["detected"] and host["target_detected"]:
                    line = self.ax.plot(
                        [x, target["x"]], 
                        [y, target["y"]], 
                        [0, target["h"]],
                        color='orange',
                        linestyle='--',
                        linewidth=1,
                        alpha=0.7
                    )
                    self.lines.extend(line)

            # 绘制目标
            if target["detected"]:
                self.target = self.ax.scatter(
                    target["x"], target["y"], target["h"],
                    c='red',
                    marker='^',
                    s=200,
                    depthshade=True,
                    edgecolors='black',
                    label='目标'
                )

            # 自动调整坐标范围
            """
            if stations or target["detected"]:
                all_x = [s["self_x"] for s in stations] + ([target["x"]] if target["detected"] else [])
                all_y = [s["self_y"] for s in stations] + ([target["y"]] if target["detected"] else [])
                all_z = [0]*len(stations) + ([target["h"]] if target["detected"] else [])
                
                pad = 5  # 边界留白
                self.ax.set_xlim(min(all_x)-pad if all_x else -10, 
                               max(all_x)+pad if all_x else 10)
                self.ax.set_ylim(min(all_y)-pad if all_y else -10, 
                               max(all_y)+pad if all_y else 10)
                self.ax.set_zlim(0, (max(all_z)+pad if all_z else 20))
            """

            # 添加图例
            handles = []
            if stations:
                handles.append(Line2D([0], [0], 
                                    marker='s', color='w', 
                                    markerfacecolor='blue', markersize=10,
                                    label='基站'))
            if target["detected"]:
                handles.append(Line2D([0], [0], 
                                    marker='^', color='w', 
                                    markerfacecolor='red', markersize=10,
                                    label='目标'))
            if handles:
                self.ax.legend(handles=handles, loc='upper right')

            self.canvas.draw()
            
        except Exception as e:
            print(f"渲染错误: {str(e)}")

    def reset_view(self):
        """重置为初始视角"""
        self.ax.view_init(elev=self.initial_view[0], azim=self.initial_view[1])
        self.canvas.draw()

    def closeEvent(self, event):
        self.timer.stop()
        event.accept()

# ================== 目标位置获取线程 ==================
class StatusFetcher(QThread):
    target_updated = pyqtSignal(dict)       # 目标信息更新信号
    error_occurred = pyqtSignal(str)        # 错误信息信号

    def __init__(self):
        super().__init__()
        self.running = True
        self.interval = 0.05  # 采集间隔（秒）20帧

    def run(self):
        while self.running:
            try:
                # 遍历所有预定义主机
                for hostname in PREDEFINED_HOSTS.copy():
                    with lock:
                        host_info = PREDEFINED_HOSTS[hostname]
                    
                    # 跳过未上线主机
                    if host_info["system_status"] == "offline":
                        continue

                    try:
                        # 建立TCP连接
                        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                            s.settimeout(3)
                            s.connect((host_info["ip"], CONTROL_PORT))

                            # 发送控制命令头
                            control_cmd = json.dumps({"command": "control"}).encode()
                            s.sendall(struct.pack("!I", len(control_cmd)) + control_cmd)

                            # 发送状态查询命令
                            status_cmd = json.dumps({"action": "get_status"}).encode()
                            s.sendall(struct.pack("!I", len(status_cmd)) + status_cmd)

                            # 读取响应长度
                            raw_len = s.recv(4)
                            if len(raw_len) != 4:
                                raise ValueError("Invalid length header")
                            resp_len = struct.unpack("!I", raw_len)[0]

                            # 读取完整响应
                            response = bytearray()
                            while len(response) < resp_len:
                                packet = s.recv(resp_len - len(response))
                                if not packet:
                                    break
                                response.extend(packet)
                            
                            # 解析JSON响应
                            json_resp = json.loads(response.decode())
                            if json_resp.get("code", 500) != 200:
                                continue

                            # 更新主机状态
                            content = json_resp.get("content", {})
                            # print(f"主机 {hostname} 状态: {content}")
                            with lock:
                                PREDEFINED_HOSTS[hostname].update({
                                    "system_status": content.get("system_status", "unknown"),
                                    "servo_x": float(content.get("ServoXAngle", 0)),
                                    "servo_y": float(content.get("ServoYAngle", 0)),
                                    "target_x": float(content.get("TargetXAngle", 0)),
                                    "target_y": float(content.get("TargetYAngle", 0)),
                                    "self_x": float(content.get("SelfX", 0)),
                                    "self_y": float(content.get("SelfY", 0)),
                                    "camera_mode": content.get("camera_mode", "visible"),
                                    "target_detected": content.get("target_detected", "no") == "yes"
                                })
                            # print(PREDEFINED_HOSTS[hostname])

                    except Exception as e:
                        # 更新主机状态为离线
                        with lock:
                            PREDEFINED_HOSTS[hostname]["system_status"] = "offline"
                            PREDEFINED_HOSTS[hostname]["target_detected"] = False
                            PREDEFINED_HOSTS[hostname]["ip"] = "127.0.01"
                        self.error_occurred.emit(f"{hostname}状态获取失败: {str(e)}")

                # 如果检测到目标，计算坐标
                self.calculate_target_position()
                time.sleep(self.interval)
                continue
                
            except Exception as e:
                self.error_occurred.emit(f"采集线程异常: {str(e)}")

            # 睡眠500ms，避免过于频繁的循环
            time.sleep(0.5)

    def calculate_target_position(self):
        """根据多基站观测数据计算目标三维坐标"""
        detected_hosts = []
    
        # 获取所有检测到目标的主机
        with lock:
            for hostname, host in PREDEFINED_HOSTS.items():
                # print(host)
                # print(host["target_x"], host["target_y"])
                if host["target_detected"] and host["ip"] != "127.0.0.1":
                    detected_hosts.append({
                        "x": host["self_x"],     # 基站X坐标
                        "y": host["self_y"],     # 基站Y坐标
                        "theta": math.radians(host["target_x"]),  # 方位角(弧度)
                        "phi": math.radians(90.0 - host["target_y"])     # 俯仰角(弧度)
                    })
                    # print(host["target_x"], host["target_y"])
        target_point = None
        num_detected = len(detected_hosts)
    
        try:
            if num_detected == 0:
                # 无目标检测
                GLOBAL_TARGET["detected"] = False
                return
            elif num_detected == 1:
                # 单基站定位：沿观测方向延伸10米
                host = detected_hosts[0]
                target_point = self.single_host_localization(host)
            else:
                # 多基站定位：最小二乘法求最优解
                target_point = self.multi_host_localization(detected_hosts)
        
            if target_point is not None:
                # 更新全局目标信息
                with lock:
                    GLOBAL_TARGET.update({
                        "detected": True,
                        "x": target_point[0],
                        "y": target_point[1],
                        "h": target_point[2],
                        "speed": self.calculate_speed(target_point)  # 需要历史数据
                    })
                self.target_updated.emit(GLOBAL_TARGET.copy())
            
        except Exception as e:
            self.error_occurred.emit(f"定位计算失败: {str(e)}")

    def single_host_localization(self, host):
        """单基站定位算法"""
        # 将球坐标转换为方向向量
        direction = np.array([
            math.cos(host["theta"]) * math.cos(host["phi"]),
            math.sin(host["theta"]) * math.cos(host["phi"]),
            math.sin(host["phi"])
        ])
        # 沿方向向量延伸3米
        distance = 0.3  # 单位：米
        target = np.array([host["x"], host["y"], 0]) + direction * distance
        # print(f"单基站定位: {host}, {target}")
    
        return target.tolist()

    def multi_host_localization(self, hosts, max_iter=100, tol=1e-4):
        """多基站定位算法（最小二乘法）"""
        # 构建方程组 AX = b
        A = []
        b = []

        for host in hosts:
            # 获取基站位置
            p0 = np.array([host["x"], host["y"], 0])

            # 获取方向向量
            direction = np.array([
                math.cos(host["theta"]) * math.cos(host["phi"]),
                math.sin(host["theta"]) * math.cos(host["phi"]),
                math.sin(host["phi"])
            ])
        
            # 构造直线参数方程：X = p0 + t*direction
            # 转换为最小二乘形式： (I - dd^T)(X - p0) = 0
            I = np.eye(3)
            ddT = np.outer(direction, direction)
            A.append(I - ddT)
            b.append(np.dot(I - ddT, p0))

        print("___________")
    
        # 拼接矩阵
        A_total = np.vstack(A)
        b_total = np.hstack(b)
    
        # 求解最小二乘问题
        X, residuals, rank, singular = np.linalg.lstsq(A_total, b_total, rcond=None)
    
        # 结果校验
        if len(residuals) == 0 or rank < 3:
            raise ValueError("矩阵奇异，无法求解")
    
        return X.tolist()

    def calculate_speed(self, new_point):
        """速度计算（需维护历史数据）"""
        # 需要实现历史位置存储
        # 此处返回0作为占位符
        return 0.0

    def stop(self):
        self.running = False
        self.wait()

# ================== 广播监听模块 ==================
class BroadcastListener(QThread):
    host_updated = pyqtSignal(dict)
    
    def __init__(self):
        super().__init__()
        self.hosts = PREDEFINED_HOSTS
        self.running = True
        
    def run(self):
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.bind(('0.0.0.0', BROADCAST_PORT))
        sock.settimeout(1)
        
        while self.running:
            try:
                data, addr = sock.recvfrom(4096)
                try:
                    info = json.loads(data.decode())
                    hostname = info.get("hostname", "")
                    ip = info.get("ip_address", addr[0])
                    with lock:
                        if hostname in self.hosts:
                            old_ip = self.hosts[hostname]["ip"]
                            if old_ip != ip:
                                # 更新主机信息
                                self.hosts[hostname]["ip"] = ip
                                self.hosts[hostname]["system_status"] = "online"
                                self.host_updated.emit({
                                    "hostname": hostname,
                                    "ip": ip,
                                    "old_ip": old_ip
                                })
                except json.JSONDecodeError:
                    continue
            except socket.timeout:
                continue
            except Exception as e:
                print(f"广播监听异常: {str(e)}")
        
        sock.close()

    def stop(self):
        self.running = False
        self.wait()

# ================== 视频接收模块 ==================
class VideoReceiver(QThread):
    frame_received = pyqtSignal(bytes)
    error_occurred = pyqtSignal(str)
    
    def __init__(self):
        super().__init__()
        self.running = False
        self.connection = None

    def run(self):
        try:
            # 建立连接
            self.connection = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.connection.connect((current_host["ip"], current_host["port"]))
            
            # 构造并发送JSON命令
            command = json.dumps({"command": "video"}).encode()
            cmd_length = struct.pack("!I", len(command))
            self.connection.sendall(cmd_length + command)
            
            # 开始接收数据
            self.running = True
            while self.running:
                # 读取帧长度前缀
                raw_len = self.connection.recv(4)
                if not raw_len:
                    break
                frame_len = struct.unpack("!I", raw_len)[0]
                
                # 读取完整帧数据
                frame_data = bytearray()
                while len(frame_data) < frame_len:
                    packet = self.connection.recv(frame_len - len(frame_data))
                    if not packet:
                        break
                    frame_data.extend(packet)
                
                if len(frame_data) == frame_len:
                    self.frame_received.emit(bytes(frame_data))
                    
        except Exception as e:
            self.error_occurred.emit(f"Connection error: {str(e)}")
        finally:
            if self.connection:
                self.connection.close()

    def stop(self):
        self.running = False
        self.wait()

# ================== 舵机控制模块 ==================
class ServoController(QThread):
    error_occurred = pyqtSignal(str)
    
    def __init__(self, host_ip, host_port):
        super().__init__()
        self.host_ip = host_ip
        self.host_port = host_port
        self.running = False
        self.connection = None
        self.step_size = 5.0       # 单次按键偏移量（单位：度）
        self.sampling_rate = 20    # 采样频率（Hz）
        
    def run(self):
        try:
            # 建立TCP连接
            self.connection = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.connection.connect((self.host_ip, self.host_port))
            
            # 发送舵机控制命令
            command = json.dumps({"command": "servo"}).encode()
            cmd_length = struct.pack("!I", len(command))
            self.connection.sendall(cmd_length + command)
            
            # 开始控制循环
            self.running = True
            interval = 1.0 / self.sampling_rate
            while self.running:
                # 计算偏移量
                x_offset = 0.0
                if keys_pressed["left"] and not keys_pressed["right"]:
                    x_offset = self.step_size
                elif keys_pressed["right"] and not keys_pressed["left"]:
                    x_offset = -self.step_size
                
                y_offset = 0.0
                if keys_pressed["up"] and not keys_pressed["down"]:
                    y_offset = -self.step_size
                elif keys_pressed["down"] and not keys_pressed["up"]:
                    y_offset = self.step_size

                # 打包数据（小端序，直接对应C++内存布局）
                data = struct.pack("<2f", x_offset, y_offset)  # 使用2f格式和<指定字节序
                try:
                    self.connection.sendall(data)
                except BrokenPipeError:
                    break
                
                # 维持采样频率
                time.sleep(interval)
                
        except Exception as e:
            self.error_occurred.emit(f"舵机控制异常: {str(e)}")
        finally:
            if self.connection:
                self.connection.close()
            self.running = False

    def stop(self):
        self.running = False
        self.wait()

# ================== 控制客户端模块 ==================
class ControlClient(QObject):
    response_received = pyqtSignal(str)
    frame_received = pyqtSignal(bytes)
    error_occurred = pyqtSignal(str)

    def send_command(self, command):
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.connect((current_host["ip"], current_host["port"]))
                # 第一阶段：发送控制标识
                phase1 = json.dumps({"command": "control"}).encode()
                s.sendall(struct.pack("!I", len(phase1)) + phase1)
                
                # 第二阶段：发送操作指令
                phase2 = json.dumps({
                    "action": command,
                    "params": {},
                }).encode()
                s.sendall(struct.pack("!I", len(phase2)) + phase2)
                
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
                
                json_response = json.loads(response.decode())
                code = json_response.get("code", 500)
                content = json_response.get("content", "")
                
                if code == 200:
                    if command == "getframenow":
                        try:
                            if isinstance(content, list):
                                self.frame_received.emit(bytes(content))
                            else:
                                raise ValueError("非法图像数据格式")
                        except Exception as e:
                            self.error_occurred.emit(f"图像数据转换失败: {str(e)}")
                    else:
                        self.response_received.emit(f"成功: {content}")
                else:
                    self.error_occurred.emit(f"错误码 {code}: {content}")

        except Exception as e:
            self.error_occurred.emit(f"发生异常: {str(e)}")

# ================== 主界面 ==================
class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.video_thread = None
        self.control_client = None
        self.servo_controller = None
        self.broadcast_listener = None
        self.start_broadcast_listener()
        self.init_ui()
        
    def init_ui(self):
        self.setWindowTitle("中-近距离视域航空目标打击阵列监控软件")
        self.setGeometry(100, 100, 1080, 720)
    
        # 主分割器（左右比例 1:4）
        main_splitter = QSplitter(Qt.Horizontal)
        
        # 左侧控制面板
        left_splitter = QSplitter(Qt.Vertical)
        
        # 服务器选择区域
        server_group = QGroupBox("监控基站选择")
        server_layout = QVBoxLayout()
        self.host_combo = QComboBox()
        self.host_combo.currentIndexChanged.connect(self.update_current_host)
        # server_layout.addWidget(QLabel("选择监控主机:"))
        server_layout.addWidget(self.host_combo)
        server_group.setLayout(server_layout)

        # 新增目标坐标区域
        coord_group = QGroupBox("目标坐标")
        coord_layout = QHBoxLayout()

        coord_display = QWidget()
        coord_display_layout = QVBoxLayout()
        
        # 创建带标签的输入行
        def create_labeled_input(label, widget):
            hbox = QHBoxLayout()
            hbox.addWidget(QLabel(label))
            hbox.addWidget(widget)
            hbox.addStretch(1)
            return hbox
            
        self.x_value = QLineEdit("0")
        self.y_value = QLineEdit("0")
        self.h_value = QLineEdit("0")

        # 设置输入框属性
        for widget in [self.x_value, self.y_value, self.h_value]:
            widget.setReadOnly(True)
            widget.setMaximumWidth(60)
            widget.setAlignment(Qt.AlignRight)

        
        # 添加带标签的输入行
        coord_display_layout.addLayout(create_labeled_input("X:", self.x_value))
        coord_display_layout.addLayout(create_labeled_input("Y:", self.y_value))
        coord_display_layout.addLayout(create_labeled_input("H:", self.h_value))
        coord_display.setLayout(coord_display_layout)
        
        # 渲染按钮
        self.render_btn = QPushButton("实时态势")
        self.render_btn.clicked.connect(self.show_render_window)
        
        coord_layout.addWidget(coord_display)
        coord_layout.addWidget(self.render_btn)
        coord_group.setLayout(coord_layout)

        # 舵机控制
        servo_group = QGroupBox("舵机遥控开关")
        servo_layout = QVBoxLayout()
        self.radio_off = QRadioButton("关闭")
        self.radio_on = QRadioButton("开启")
        # 默认选中"关闭"
        self.radio_off.setChecked(True)
        # servo_layout.addWidget(QLabel("舵机控制开关:"))
        servo_layout.addWidget(self.radio_on)
        servo_layout.addWidget(self.radio_off)
        servo_group.setLayout(servo_layout)
        
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
        
        # 组合左侧布局
        left_splitter.addWidget(server_group)
        left_splitter.addWidget(coord_group)  # 新增坐标区域
        left_splitter.addWidget(servo_group)
        left_splitter.addWidget(video_group)
        left_splitter.addWidget(control_group)
        left_splitter.setSizes([80, 120, 100, 80, 200])  # 调整尺寸分配
        left_splitter.setCollapsible(0, False)
        
        # 右侧显示区域
        right_splitter = QSplitter(Qt.Vertical)
        self.video_label = QLabel()
        self.video_label.setAlignment(Qt.AlignCenter)
        self.video_label.setStyleSheet("background-color: black;")
        self.video_label.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        
        self.log_output = QTextEdit()
        self.log_output.setReadOnly(True)
        self.log_output.setMinimumHeight(150)
        
        right_splitter.addWidget(self.video_label)
        right_splitter.addWidget(self.log_output)
        right_splitter.setSizes([600, 200])
        
        # 组合主界面
        main_splitter.addWidget(left_splitter)
        main_splitter.addWidget(right_splitter)
        main_splitter.setSizes([240, 960])
        main_splitter.setStretchFactor(0, 1)
        main_splitter.setStretchFactor(1, 4)
        
        self.setCentralWidget(main_splitter)
    
        # 连接信号
        self.start_btn.clicked.connect(self.start_video)
        self.stop_btn.clicked.connect(self.stop_video)
        self.frame_btn.clicked.connect(lambda: self.send_control("getframenow"))
        self.sleep_btn.clicked.connect(lambda: self.send_control("sleep"))
        self.wakeup_btn.clicked.connect(lambda: self.send_control("wakeup"))
        self.power_btn.clicked.connect(lambda: self.send_control("poweroff"))
        self.radio_on.clicked.connect(self.toggle_servo_control)
        self.radio_off.clicked.connect(self.toggle_servo_control)
        
        # 初始化下拉框
        self.update_host_combo()

        # 新增状态采集线程
        self.status_fetcher = StatusFetcher()
        self.status_fetcher.target_updated.connect(self.handle_target_update)
        self.status_fetcher.error_occurred.connect(self.log_message)
        self.status_fetcher.start()

    def show_render_window(self):
        # 单例模式，避免重复打开窗口
        if not hasattr(self, 'render_window') or not self.render_window.isVisible():
            self.render_window = RenderWindow()
        self.render_window.show()
        self.render_window.activateWindow()
        
    def start_broadcast_listener(self):
        self.broadcast_listener = BroadcastListener()
        self.broadcast_listener.host_updated.connect(self.handle_host_update)
        self.broadcast_listener.start()
        
    def handle_host_update(self, host_info):
        hostname = host_info["hostname"]
        new_ip = host_info["ip"]
        old_ip = host_info["old_ip"]
        
        log_msg = f"主机状态更新: {hostname} "
        if new_ip != old_ip:
            if old_ip == "127.0.0.1":
                log_msg += f"已上线 ({new_ip})"
            else:
                log_msg += f"IP变更 {old_ip} → {new_ip}"
            self.log_message(log_msg)
            
        self.update_host_combo()
        
    def update_host_combo(self):
        current_text = self.host_combo.currentText()
        self.host_combo.clear()
        with lock:
            for hostname, info in self.broadcast_listener.hosts.items():
                ip = info["ip"]
                # 仅显示在线主机
                status = f"{hostname} [{ip}]" if ip != "127.0.0.1" else hostname
                self.host_combo.addItem(status, userData=hostname)
            
        # 恢复之前的选择
        index = self.host_combo.findText(current_text)
        if index >= 0:
            self.host_combo.setCurrentIndex(index)
        elif self.host_combo.count() > 0:
            self.host_combo.setCurrentIndex(0)
            self.update_current_host(0)
            
    def update_current_host(self, index):
        if index >= 0:
            hostname = self.host_combo.itemData(index)
            # 如果切换的主机与当前不同且舵机在运行
            if hostname != current_host["hostname"] and self.radio_on.isChecked():
                self.radio_off.setChecked(True)
                if self.servo_controller:
                    self.servo_controller.stop()
                    self.log_message("主机切换，舵机控制已停止")
            current_host["hostname"] = hostname
            current_host["ip"] = self.broadcast_listener.hosts.get(hostname)["ip"]
            # self.log_message(f"当前主机: {hostname} ({current_host['ip']})")

    def handle_target_update(self, target_info):
        """更新目标坐标显示"""
        self.x_value.setText(f"{target_info['x']:.2f}")
        self.y_value.setText(f"{target_info['y']:.2f}")
        self.h_value.setText(f"{target_info['h']:.2f}")
        
        # 更新全局目标信息
        #with lock:
            #GLOBAL_TARGET.update(target_info)
            
    def log_message(self, message):
        timestamp = datetime.now().strftime("%H:%M:%S")
        self.log_output.append(f"[{timestamp}] {message}")
        self.log_output.verticalScrollBar().setValue(
            self.log_output.verticalScrollBar().maximum())

    def toggle_servo_control(self):
        """处理舵机控制开关状态变化"""
        if self.radio_on.isChecked():
            # 停止现有控制
            if self.servo_controller and self.servo_controller.isRunning():
                self.servo_controller.stop()
                
            # 创建新控制器
            self.servo_controller = ServoController(
                current_host["ip"], 
                CONTROL_PORT
            )
            self.servo_controller.error_occurred.connect(self.log_message)
            self.servo_controller.start()
            self.log_message("舵机控制已启动")
        else:
            if self.servo_controller:
                self.servo_controller.stop()
                self.log_message("舵机控制已停止")
        
    def start_video(self):
        if self.video_thread and self.video_thread.isRunning():
            return
            
        self.video_thread = VideoReceiver()
        self.video_thread.frame_received.connect(self.show_video_frame)
        self.video_thread.error_occurred.connect(self.log_message)
        self.video_thread.start()
        
        self.start_btn.setEnabled(False)
        self.stop_btn.setEnabled(True)
        self.log_message(f"正在连接 {current_host['hostname']}...")
        
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
            nparr = np.frombuffer(frame_data, np.uint8)
            img = cv2.imdecode(nparr, cv2.IMREAD_COLOR)
            if img is not None:
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
        self.control_client = ControlClient()
        self.control_client.response_received.connect(self.log_message)
        self.control_client.frame_received.connect(self.save_frame)
        self.control_client.error_occurred.connect(self.log_message)
        
        thread = threading.Thread(
            target=self.control_client.send_command,
            args=(command,)
        )
        thread.start()
        
        self.log_message(f"已发送控制指令: {command}")
        
    def save_frame(self, frame_data):
        try:
            nparr = np.frombuffer(frame_data, np.uint8)
            img = cv2.imdecode(nparr, cv2.IMREAD_COLOR)
            img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
            
            if img is None:
                raise ValueError("解码失败：无效的图像数据")
    
            save_dir = "frames"
            os.makedirs(save_dir, exist_ok=True)
            
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S_%f")[:-3]
            filename = f"{save_dir}/{timestamp}.png"
            
            compression_params = [cv2.IMWRITE_PNG_COMPRESSION, 0]
            
            if not cv2.imwrite(filename, img, compression_params):
                raise RuntimeError("PNG图像写入失败")
                
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

    def closeEvent(self, event):
        if self.broadcast_listener:
            self.broadcast_listener.stop()
        if self.video_thread and self.video_thread.isRunning():
            self.video_thread.stop()
        if self.status_fetcher:
            self.status_fetcher.stop()
        event.accept()

if __name__ == "__main__":
    # 创建监听器但不阻塞
    listener = keyboard.Listener(on_press=on_press, on_release=on_release)
    listener.daemon = True  # 设置为守护线程（主程序退出时自动结束）
    listener.start()
    
    QApplication.setAttribute(Qt.AA_EnableHighDpiScaling, True)
    QApplication.setAttribute(Qt.AA_UseHighDpiPixmaps, True)
    app = QApplication(sys.argv)
    font = app.font()
    font.setPointSize(12)
    app.setFont(font)
    
    window = MainWindow()
    window.show()
    sys.exit(app.exec_())
