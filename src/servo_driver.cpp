#include "servo_driver.h"  // 包含自定义的ServoDriver类的定义

ServoDriver::ServoDriver(const std::string& device):device(device){
    servoIDs.push_back(0x01); // 默认包含ID 1
}

ServoDriver::~ServoDriver() {
    close(serialPort); // 关闭串口
}

bool ServoDriver::openPort() {
    serialPort = open(this->device.c_str(), O_RDWR | O_NOCTTY); // 打开串口设备 去掉O_NDELAY，以便设置为阻塞模式
    // O_RDWR:以读写模式打开文件 | O_NOCTTY:不将文件描述符设置为控制终端
    if (serialPort == -1) {
        std::cerr << "Failed to open serial port device: " << device << std::endl;
        return false;
    }

    struct termios options;
    tcgetattr(serialPort, &options); // 获取当前串口设置

    options.c_cflag = B115200 | CS8 | CLOCAL | CREAD; // 设置波特率为115200，8位数据位，无校验，本地连接，接收使能
    options.c_iflag = IGNPAR; // 忽略奇偶校验错误
    options.c_oflag = 0; // 输出模式为原始数据
    options.c_lflag = 0; // 本地模式为原始数据
    // 设置超时时间为1秒（10次尝试，每次100毫秒）
    options.c_cc[VTIME] = 10; // 10 * 100ms = 1s
    options.c_cc[VMIN] = 0; // 最小字符数为0，表示超时模式

    tcflush(serialPort, TCIFLUSH); // 清空输入缓冲区
    tcsetattr(serialPort, TCSANOW, &options); // 立即应用新的串口设置

    return true;
}

inline bool ServoDriver::sendCommand(const std::vector<uint8_t>& data) {
    // 打印发送的指令内容
    /*
    std::cout << "Sending command: ";
    for (const auto& byte : data) {
        std::cout << std::hex << std::setw(2) << static_cast<int>(byte) << " ";
    }
    std::cout << std::endl;
    */

    // 发送指令
    if (write(serialPort, data.data(), data.size()) != data.size()) {
        std::cerr << "Sending instructions failed." << std::endl;
        return false;
    }
    return true;
}

inline std::vector<uint8_t> ServoDriver::generateCommand(uint8_t id, uint8_t instruction, const std::vector<uint8_t>& params) {
    std::vector<uint8_t> command = { 0xFF, 0xFF }; // 字头
    command.push_back(id); // ID号
    uint8_t length = params.size() + 2; // 数据长度
    command.push_back(length);
    command.push_back(instruction); // 指令类型
    command.insert(command.end(), params.begin(), params.end()); // 参数
    command.push_back(calculateChecksum(command)); // 校验和

    // 打印命令参数及其含义
    /*
    std::cout << "Command Explanation: ";
    std::cout << std::left << std::setw(12) << "Header1" << std::setw(12) << "Header2" << std::setw(12) << "ID" << std::setw(12) << "Length" << std::setw(12) << "Instruction";
    for (size_t i = 0; i < params.size(); ++i) {
        std::cout << std::setw(12) << ("Param" + std::to_string(i + 1));
    }
    std::cout << std::setw(10) << "Checksum" << std::endl;

    std::cout << "Generated Command:   ";
    for (size_t i = 0; i < command.size(); ++i) {
        std::stringstream ss;
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(command[i]);
        std::cout << std::left << std::setw(12) << ("0x" + ss.str());
    }
    std::cout << std::endl;
    */
    return command;
}

inline std::tuple<uint8_t, uint8_t, std::vector<uint8_t>> ServoDriver::parseResponse(const std::vector<uint8_t>& response) {
    if (response.size() < 6 || response[0] != 0xFF || response[1] != 0xF5) { // 检查应答包格式
        std::cerr << "Response package format error." << std::endl;
        return std::make_tuple(0, 0, std::vector<uint8_t>());
    }

    uint8_t id = response[2]; // 获取舵机ID
    //uint8_t length = response[3]; // 获取数据长度
    uint8_t status = response[4]; // 获取舵机状态
    std::vector<uint8_t> params(response.begin() + 5, response.end() - 1); // 获取参数
    uint8_t checksum = response.back(); // 获取校验和

    std::vector<uint8_t> checksumData(response.begin(), response.end() - 1); // 校验和计算数据
    if (checksum != calculateChecksum(checksumData)) { // 校验和验证
        std::cerr << "Checksum error." << std::endl;
        return std::make_tuple(0, 0, std::vector<uint8_t>());
    }
    return std::make_tuple(id, status, params); // 返回解析结果
}

uint8_t ServoDriver::ping(uint8_t id) {
    auto command = generateCommand(id, 0x01, {}); // 生成PING指令
    if (!sendCommand(command)) { // 发送指令
        return 0xFF;
    }

    std::vector<uint8_t> response(6); // 应答包大小为6字节
    if (read(serialPort, response.data(), response.size()) != response.size()) { // 读取应答包
        std::cerr << "Failed to read the response package." << std::endl;
        return 0xFF;
    }

    auto [id_, status, params] = parseResponse(response); // 解析应答包
    if (id_ != id) { // 检查应答包ID
        std::cerr << "Response package ID error." << std::endl;
        return 0xFF;
    }

    return status;
}

inline std::vector<uint8_t> ServoDriver::readData(uint8_t id, uint8_t address, uint8_t length) {
    // id：舵机ID，address：需要读取数据的首地址（读指令都只有一位），
    // length 需要读取数据的字节数
    auto command = generateCommand(id, 0x02, { address, length }); // 生成读指令
    if (!sendCommand(command)) { // 发送指令
        std::cerr << "Failed to send command. Error: " << std::strerror(errno) << std::endl;
        return {};
    }

    std::vector<uint8_t> response(length + 6); // 应答包大小为length + 6字节
    if (read(serialPort, response.data(), response.size()) != response.size()) { // 读取应答包
        std::cerr << "Failed to read the response package. Error: " << std::strerror(errno) << std::endl;
        return {};
    }

    auto [id_, status, params] = parseResponse(response); // 解析应答包
    if (id_ != id) { // 检查应答包ID
        std::cerr << "Response package ID error." << std::endl;
        return {};
    }

    return params; // 返回读取的数据
}

inline bool ServoDriver::writeData(uint8_t id, uint8_t address, const std::vector<uint8_t>& data) {
    std::vector<uint8_t> params = { address }; // 参数包含地址
    params.insert(params.end(), data.begin(), data.end()); // 添加数据
    auto command = generateCommand(id, 0x03, params); // 生成写指令
    return sendCommand(command); // 发送指令
}

bool ServoDriver::regWrite(uint8_t id, uint8_t address, const std::vector<uint8_t>& data) {
    std::vector<uint8_t> params = { address }; // 参数包含地址
    params.insert(params.end(), data.begin(), data.end()); // 添加数据
    auto command = generateCommand(id, 0x04, params); // 生成异步写指令
    return sendCommand(command);
}

bool ServoDriver::action() {
    auto command = generateCommand(0xFE, 0x05, {}); // 生成ACTION指令
    return sendCommand(command); // 发送指令
}

bool ServoDriver::reset(uint8_t id) {
    auto command = generateCommand(id, 0x06, {}); // 生成RESET指令
    return sendCommand(command); // 发送指令
}

bool ServoDriver::syncWrite(uint8_t address, const std::vector<uint8_t>& data) {
    if (data.size() % 2 != 0) { // 检查数据格式
        std::cerr << "SYNC WRITE data format error." << std::endl;
        return false;
    }
    std::vector<uint8_t> params = { address, static_cast<uint8_t>(data.size() / 2) }; // 参数包
    params.insert(params.end(), data.begin(), data.end()); // 添加数据
    auto command = generateCommand(0xFE, 0x83, params); // 生成同步写指令
    return sendCommand(command); // 发送指令
}

inline uint8_t ServoDriver::calculateChecksum(const std::vector<uint8_t>& data) {
    uint8_t checksum = 0;
    for (size_t i = 2; i < data.size(); ++i) { // 从第三位开始计算校验和
        checksum += data[i];
    }
    return (~checksum) & 0xFF; // 取反并截取低8位
}

std::string ServoDriver::getSoftwareVersion(uint8_t id) {
    // 读取软件版本信息，地址为0x03和0x04，共2字节
    std::vector<uint8_t> versionData = readData(id, ADDR_SOFTWARE_VERSION, LEN_SOFTWARE_VERSION);

    // 检查读取的数据是否有效
    if (versionData.size() != 2) {
        std::cerr << "Failed to read software version." << std::endl;
        return "";
    }

    // 将读取的数据转换为版本字符串
    uint8_t majorVersion = versionData[0];
    uint8_t minorVersion = versionData[1];
    std::stringstream versionStream;
    versionStream << "v" << static_cast<int>(majorVersion) << "." << std::setw(2) << std::setfill('0') << static_cast<int>(minorVersion);

    return versionStream.str();
}

bool ServoDriver::setAngleLimits(uint8_t id, float minAngle, float maxAngle) {
    // 设置角度限制
    const float conversionFactor = 4096.0f / SERVO_ANGLE_RANGE; // 提前计算转换系数

    // 检查输入角度是否在有效范围内
    if (minAngle < -SERVO_ANGLE_HALF_RANGE || minAngle > SERVO_ANGLE_HALF_RANGE || maxAngle < -SERVO_ANGLE_HALF_RANGE || maxAngle > SERVO_ANGLE_HALF_RANGE || minAngle > maxAngle) {
        std::cerr << "Invalid angle limits." << std::endl;
        return false;
    }

    // 将角度转换为舵机内部的角度值（0-4095）
    uint16_t minAngleInternal = static_cast<uint16_t>((minAngle + SERVO_ANGLE_HALF_RANGE) * conversionFactor);
    uint16_t maxAngleInternal = static_cast<uint16_t>((maxAngle + SERVO_ANGLE_HALF_RANGE) * conversionFactor);

    // 将内部角度值拆分为两个字节
    uint8_t minAngleHigh = static_cast<uint8_t>((minAngleInternal >> 8) & 0xFF);
    uint8_t minAngleLow = static_cast<uint8_t>(minAngleInternal & 0xFF);
    uint8_t maxAngleHigh = static_cast<uint8_t>((maxAngleInternal >> 8) & 0xFF);
    uint8_t maxAngleLow = static_cast<uint8_t>(maxAngleInternal & 0xFF);

    // 写入最小角度限制
    std::vector<uint8_t> minAngleData = { minAngleHigh, minAngleLow };
    if (!writeData(id, ADDR_MIN_ANGLE_LIMIT, minAngleData)) {
        std::cerr << "Failed to set minimum angle limit." << std::endl;
        return false;
    }

    // 写入最大角度限制
    std::vector<uint8_t> maxAngleData = { maxAngleHigh, maxAngleLow };
    if (!writeData(id, ADDR_MAX_ANGLE_LIMIT, maxAngleData)) {
        std::cerr << "Failed to set maximum angle limit." << std::endl;
        return false;
    }

    return true;
}

bool ServoDriver::setTargetPosition(uint8_t id, float angle) {
    // 检查角度范围
    if (angle < -SERVO_ANGLE_HALF_RANGE || angle > SERVO_ANGLE_HALF_RANGE) {
        std::cerr << "Angle out of range. Must be between -135 and 135 degrees." << std::endl;
        return false;
    }

    // 计算转换系数
    float conversionFactor = 4096.0f / SERVO_ANGLE_RANGE;

    // 将角度转换为舵机需要的0~4095范围内的值
    uint16_t targetPosition = static_cast<uint16_t>((angle + SERVO_ANGLE_HALF_RANGE) * conversionFactor);

    // 将目标位置转换为两个字节的数据
    uint8_t highByte = static_cast<uint8_t>(targetPosition >> 8);
    uint8_t lowByte = static_cast<uint8_t>(targetPosition & 0xFF);

    // 发送写指令
    return writeData(id, ADDR_TARGET_POSITION, { highByte, lowByte });
}

float ServoDriver::getCurrentPosition(uint8_t id) {
    // 读取当前位置，地址为0x38和0x39，共2字节
    std::vector<uint8_t> positionData = readData(id, ADDR_CURRENT_POSITION, LEN_CURRENT_POSITION);

    // 检查读取的数据是否有效
    if (positionData.size() != 2) {
        std::cerr << "Failed to read current position." << std::endl;
        return -999.0f; // 返回一个无效值
    }

    // 将读取的数据合并为16位整数
    uint16_t currentPosition = (static_cast<uint16_t>(positionData[0]) << 8) | positionData[1];

    // 计算转换系数
    float conversionFactor = SERVO_ANGLE_RANGE / 4096.0f;

    // 将当前位置转换为角度值
    float angle = (currentPosition * conversionFactor) - 135.0f;

    return angle;
}


bool ServoDriver::setMidPointAdjustment(uint8_t id, float angle) {
    // 设置中立位
    if (angle < -SERVO_ANGLE_HALF_RANGE || angle > SERVO_ANGLE_HALF_RANGE) { // 检查角度范围
        std::cerr << "Angle out of range. Must be between -135 and 135 degrees." << std::endl;
        return false;
    }

    // 计算转换系数
    float conversionFactor = 4096.0f / SERVO_ANGLE_RANGE;

    // 将角度转换为舵机需要的中位调整值（-2048到2048）
    int16_t midPointAdjustment = static_cast<int16_t>(angle * conversionFactor);

    // 将中位调整值转换为两个字节的数据
    uint8_t highByte = static_cast<uint8_t>(midPointAdjustment >> 8);
    uint8_t lowByte = static_cast<uint8_t>(midPointAdjustment & 0xFF);

    // 发送写指令
    return writeData(id, ADDR_CENTER_ADJUSTMENT, { highByte, lowByte });
}

bool ServoDriver::setAndWaitForPosition(uint8_t servoID, float targetPosition) {
    bool success = setTargetPosition(servoID, targetPosition);
    if (!success) {
        std::cerr << "Failed to set target position." << std::endl;
        return false;
    }
    //std::cout << "Target position set to " << targetPosition << " degrees." << std::endl;
    while (true) {
        float currentPosition = getCurrentPosition(servoID);
        //std::cout << "Current Position: " << currentPosition << " degrees" << std::endl;

        if (std::abs(currentPosition - targetPosition) <= 1.0f) {
            //std::cout << "Position reached within 1.0 degrees." << std::endl;
            break;
        }
        // 等待一段时间再读取当前位置
        usleep(1000); // 等待1毫秒
    }
    return true;
}
