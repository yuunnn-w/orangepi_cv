#ifndef SERVO_DRIVER_H
#define SERVO_DRIVER_H

#include <fcntl.h>         // 包含文件控制选项，用于打开串口设备
#include <termios.h>       // 包含终端I/O接口的定义，用于配置串口参数
#include <unistd.h>        // 包含POSIX标准库函数，如write、close等
#include <iostream>        // 包含标准输入输出流，用于输出错误信息
#include <vector>          // 包含标准库的动态数组容器，用于存储发送的数据
#include <tuple>           // 包含标准库的元组容器，用于存储多个值
#include <iomanip>         // 包含输入输出流操纵符，用于设置输出格式
#include <cerrno>          // 用于检查函数调用发生的错误代码
#include <cstring>         // 提供了一些用于处理 C 风格字符串的函数
#include <cmath>
#include <cstdint>

#define SERVO_ANGLE_RANGE 270.0 //舵机最大转角
#define SERVO_ANGLE_HALF_RANGE 135.0 //舵机最大转角的一半

class ServoDriver {
public:
    // 构造函数，初始化舵机ID列表
    ServoDriver(const std::string& device); //device 串口设备地址，例如 "/dev/ttyUSB0"
    ~ServoDriver(); // 析构函数，关闭串口

    bool openPort(); //打开串口 return 是否成功打开串口
    inline bool sendCommand(const std::vector<uint8_t>& data); // 发送指令 return 是否成功发送
    inline std::vector<uint8_t> generateCommand(uint8_t id, uint8_t instruction, const std::vector<uint8_t>& params); // 生成指令
    inline std::tuple<uint8_t, uint8_t, std::vector<uint8_t>> parseResponse(const std::vector<uint8_t>& response); //  解析应答包
    uint8_t ping(uint8_t id);// PING指令 return 是否成功
    inline std::vector<uint8_t> readData(uint8_t id, uint8_t address, uint8_t length); // READ DATA指令 
    // address 读取数据的起始地址
    // length 读取数据的长度
    // return 读取到的数据
    inline bool writeData(uint8_t id, uint8_t address, const std::vector<uint8_t>& data); // WRITE DATA指令
    bool regWrite(uint8_t id, uint8_t address, const std::vector<uint8_t>& data); // REG WRITE指令
    bool action(); // ACTION指令
    bool reset(uint8_t id); // RESET指令
    bool syncWrite(uint8_t address, const std::vector<uint8_t>& data); // SYNC WRITE指令

    std::string getSoftwareVersion(uint8_t id); // 获取软件版本
    bool setAngleLimits(uint8_t id, float minAngle, float maxAngle); // 设置角度限制
    bool setTargetPosition(uint8_t id, float angle);  // 设置目标位置
    float getCurrentPosition(uint8_t id); // 读取当前位置
    bool setMidPointAdjustment(uint8_t id, float angle); //设置中立位
    bool setAndWaitForPosition(uint8_t servoID, float targetPosition); //移动到目标位置并等待完成

private:
    int serialPort; // 串口句柄
    std::vector<uint8_t> servoIDs; // 舵机ID列表
    std::string device; // 串口设备（串口挂载地址）
    std::vector<uint8_t> buffer; // 接收缓冲区
    uint8_t calculateChecksum(const std::vector<uint8_t>& data); // 计算校验和 return 校验和
};


// 软件版本
const uint8_t ADDR_SOFTWARE_VERSION = 0x03;
const uint8_t LEN_SOFTWARE_VERSION = 2;

// 舵机ID
const uint8_t ADDR_SERVO_ID = 0x05;
const uint8_t LEN_SERVO_ID = 1;

// 堵转保护时间
const uint8_t ADDR_STALL_PROTECTION_TIME = 0x06;
const uint8_t LEN_STALL_PROTECTION_TIME = 1;

// 最小角度限制
const uint8_t ADDR_MIN_ANGLE_LIMIT = 0x09;
const uint8_t LEN_MIN_ANGLE_LIMIT = 2;

// 最大角度限制
const uint8_t ADDR_MAX_ANGLE_LIMIT = 0x0B;
const uint8_t LEN_MAX_ANGLE_LIMIT = 2;

// 温度保护阀值
const uint8_t ADDR_TEMPERATURE_PROTECTION_THRESHOLD = 0x0D;
const uint8_t LEN_TEMPERATURE_PROTECTION_THRESHOLD = 1;

// 最大扭矩
const uint8_t ADDR_MAX_TORQUE = 0x10;
const uint8_t LEN_MAX_TORQUE = 2;

// 卸载条件
const uint8_t ADDR_UNLOAD_CONDITION = 0x13;
const uint8_t LEN_UNLOAD_CONDITION = 1;

// 中位调整
const uint8_t ADDR_CENTER_ADJUSTMENT = 0x14;
const uint8_t LEN_CENTER_ADJUSTMENT = 2;

// 设定目标位置一
const uint8_t ADDR_SET_TARGET_POSITION_1 = 0x16;
const uint8_t LEN_SET_TARGET_POSITION_1 = 2;

// 设定目标位置二
const uint8_t ADDR_SET_TARGET_POSITION_2 = 0x18;
const uint8_t LEN_SET_TARGET_POSITION_2 = 2;

// 设定目标位置三
const uint8_t ADDR_SET_TARGET_POSITION_3 = 0x1A;
const uint8_t LEN_SET_TARGET_POSITION_3 = 2;

// 舵机电机模式
const uint8_t ADDR_SERVO_MOTOR_MODE = 0x1C;
const uint8_t LEN_SERVO_MOTOR_MODE = 1;

// 电机模式方向
const uint8_t ADDR_MOTOR_MODE_DIRECTION = 0x1D;
const uint8_t LEN_MOTOR_MODE_DIRECTION = 1;

// 波特率
const uint8_t ADDR_BAUDRATE = 0x1E;
const uint8_t LEN_BAUDRATE = 1;

// PID 位置环参数设定
const uint8_t ADDR_PID_POSITION_P = 0x1F;
const uint8_t ADDR_PID_POSITION_I = 0x20;
const uint8_t ADDR_PID_POSITION_D = 0x21;
const uint8_t LEN_PID_POSITION = 1;

// PID 速度环参数设定
const uint8_t ADDR_PID_SPEED_P = 0x22;
const uint8_t ADDR_PID_SPEED_I = 0x23;
const uint8_t ADDR_PID_SPEED_D = 0x24;
const uint8_t LEN_PID_SPEED = 1;

// 扭矩开关
const uint8_t ADDR_TORQUE_SWITCH = 0x28;
const uint8_t LEN_TORQUE_SWITCH = 1;

// 目标位置
const uint8_t ADDR_TARGET_POSITION = 0x2A;
const uint8_t LEN_TARGET_POSITION = 2;

// 运行时间
const uint8_t ADDR_RUN_TIME = 0x2C;
const uint8_t LEN_RUN_TIME = 2;

// 当前位置
const uint8_t ADDR_CURRENT_POSITION = 0x38;
const uint8_t LEN_CURRENT_POSITION = 2;

// 运行保存的目标位置
const uint8_t ADDR_RUN_SAVED_TARGET_POSITION = 0x3C;
const uint8_t LEN_RUN_SAVED_TARGET_POSITION = 1;

// REG WRITE 标志
const uint8_t ADDR_REG_WRITE_FLAG = 0x40;
const uint8_t LEN_REG_WRITE_FLAG = 1;

// 速度调整
const uint8_t ADDR_SPEED_ADJUSTMENT = 0x41;
const uint8_t LEN_SPEED_ADJUSTMENT = 2;


#endif // SERVO_DRIVER_H

