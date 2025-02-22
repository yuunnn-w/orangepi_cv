#include "servo_driver.h"  // �����Զ����ServoDriver��Ķ���

ServoDriver::ServoDriver(const std::string& device):device(device){
    servoIDs.push_back(0x01); // Ĭ�ϰ���ID 1
}

ServoDriver::~ServoDriver() {
    close(serialPort); // �رմ���
}

bool ServoDriver::openPort() {
    serialPort = open(this->device.c_str(), O_RDWR | O_NOCTTY); // �򿪴����豸 ȥ��O_NDELAY���Ա�����Ϊ����ģʽ
    // O_RDWR:�Զ�дģʽ���ļ� | O_NOCTTY:�����ļ�����������Ϊ�����ն�
    if (serialPort == -1) {
        std::cerr << "Failed to open serial port device: " << device << std::endl;
        return false;
    }

    struct termios options;
    tcgetattr(serialPort, &options); // ��ȡ��ǰ��������

    options.c_cflag = B115200 | CS8 | CLOCAL | CREAD; // ���ò�����Ϊ115200��8λ����λ����У�飬�������ӣ�����ʹ��
    options.c_iflag = IGNPAR; // ������żУ�����
    options.c_oflag = 0; // ���ģʽΪԭʼ����
    options.c_lflag = 0; // ����ģʽΪԭʼ����
    // ���ó�ʱʱ��Ϊ1�루10�γ��ԣ�ÿ��100���룩
    options.c_cc[VTIME] = 10; // 10 * 100ms = 1s
    options.c_cc[VMIN] = 0; // ��С�ַ���Ϊ0����ʾ��ʱģʽ

    tcflush(serialPort, TCIFLUSH); // ������뻺����
    tcsetattr(serialPort, TCSANOW, &options); // ����Ӧ���µĴ�������

    return true;
}

inline bool ServoDriver::sendCommand(const std::vector<uint8_t>& data) {
    // ��ӡ���͵�ָ������
    /*
    std::cout << "Sending command: ";
    for (const auto& byte : data) {
        std::cout << std::hex << std::setw(2) << static_cast<int>(byte) << " ";
    }
    std::cout << std::endl;
    */

    // ����ָ��
    if (write(serialPort, data.data(), data.size()) != data.size()) {
        std::cerr << "Sending instructions failed." << std::endl;
        return false;
    }
    return true;
}

inline std::vector<uint8_t> ServoDriver::generateCommand(uint8_t id, uint8_t instruction, const std::vector<uint8_t>& params) {
    std::vector<uint8_t> command = { 0xFF, 0xFF }; // ��ͷ
    command.push_back(id); // ID��
    uint8_t length = params.size() + 2; // ���ݳ���
    command.push_back(length);
    command.push_back(instruction); // ָ������
    command.insert(command.end(), params.begin(), params.end()); // ����
    command.push_back(calculateChecksum(command)); // У���

    // ��ӡ����������京��
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
    if (response.size() < 6 || response[0] != 0xFF || response[1] != 0xF5) { // ���Ӧ�����ʽ
        std::cerr << "Response package format error." << std::endl;
        return std::make_tuple(0, 0, std::vector<uint8_t>());
    }

    uint8_t id = response[2]; // ��ȡ���ID
    //uint8_t length = response[3]; // ��ȡ���ݳ���
    uint8_t status = response[4]; // ��ȡ���״̬
    std::vector<uint8_t> params(response.begin() + 5, response.end() - 1); // ��ȡ����
    uint8_t checksum = response.back(); // ��ȡУ���

    std::vector<uint8_t> checksumData(response.begin(), response.end() - 1); // У��ͼ�������
    if (checksum != calculateChecksum(checksumData)) { // У�����֤
        std::cerr << "Checksum error." << std::endl;
        return std::make_tuple(0, 0, std::vector<uint8_t>());
    }
    return std::make_tuple(id, status, params); // ���ؽ������
}

uint8_t ServoDriver::ping(uint8_t id) {
    auto command = generateCommand(id, 0x01, {}); // ����PINGָ��
    if (!sendCommand(command)) { // ����ָ��
        return 0xFF;
    }

    std::vector<uint8_t> response(6); // Ӧ�����СΪ6�ֽ�
    if (read(serialPort, response.data(), response.size()) != response.size()) { // ��ȡӦ���
        std::cerr << "Failed to read the response package." << std::endl;
        return 0xFF;
    }

    auto [id_, status, params] = parseResponse(response); // ����Ӧ���
    if (id_ != id) { // ���Ӧ���ID
        std::cerr << "Response package ID error." << std::endl;
        return 0xFF;
    }

    return status;
}

inline std::vector<uint8_t> ServoDriver::readData(uint8_t id, uint8_t address, uint8_t length) {
    // id�����ID��address����Ҫ��ȡ���ݵ��׵�ַ����ָ�ֻ��һλ����
    // length ��Ҫ��ȡ���ݵ��ֽ���
    auto command = generateCommand(id, 0x02, { address, length }); // ���ɶ�ָ��
    if (!sendCommand(command)) { // ����ָ��
        std::cerr << "Failed to send command. Error: " << std::strerror(errno) << std::endl;
        return {};
    }

    std::vector<uint8_t> response(length + 6); // Ӧ�����СΪlength + 6�ֽ�
    if (read(serialPort, response.data(), response.size()) != response.size()) { // ��ȡӦ���
        std::cerr << "Failed to read the response package. Error: " << std::strerror(errno) << std::endl;
        return {};
    }

    auto [id_, status, params] = parseResponse(response); // ����Ӧ���
    if (id_ != id) { // ���Ӧ���ID
        std::cerr << "Response package ID error." << std::endl;
        return {};
    }

    return params; // ���ض�ȡ������
}

inline bool ServoDriver::writeData(uint8_t id, uint8_t address, const std::vector<uint8_t>& data) {
    std::vector<uint8_t> params = { address }; // ����������ַ
    params.insert(params.end(), data.begin(), data.end()); // �������
    auto command = generateCommand(id, 0x03, params); // ����дָ��
    return sendCommand(command); // ����ָ��
}

bool ServoDriver::regWrite(uint8_t id, uint8_t address, const std::vector<uint8_t>& data) {
    std::vector<uint8_t> params = { address }; // ����������ַ
    params.insert(params.end(), data.begin(), data.end()); // �������
    auto command = generateCommand(id, 0x04, params); // �����첽дָ��
    return sendCommand(command);
}

bool ServoDriver::action() {
    auto command = generateCommand(0xFE, 0x05, {}); // ����ACTIONָ��
    return sendCommand(command); // ����ָ��
}

bool ServoDriver::reset(uint8_t id) {
    auto command = generateCommand(id, 0x06, {}); // ����RESETָ��
    return sendCommand(command); // ����ָ��
}

bool ServoDriver::syncWrite(uint8_t address, const std::vector<uint8_t>& data) {
    if (data.size() % 2 != 0) { // ������ݸ�ʽ
        std::cerr << "SYNC WRITE data format error." << std::endl;
        return false;
    }
    std::vector<uint8_t> params = { address, static_cast<uint8_t>(data.size() / 2) }; // ������
    params.insert(params.end(), data.begin(), data.end()); // �������
    auto command = generateCommand(0xFE, 0x83, params); // ����ͬ��дָ��
    return sendCommand(command); // ����ָ��
}

inline uint8_t ServoDriver::calculateChecksum(const std::vector<uint8_t>& data) {
    uint8_t checksum = 0;
    for (size_t i = 2; i < data.size(); ++i) { // �ӵ���λ��ʼ����У���
        checksum += data[i];
    }
    return (~checksum) & 0xFF; // ȡ������ȡ��8λ
}

std::string ServoDriver::getSoftwareVersion(uint8_t id) {
    // ��ȡ����汾��Ϣ����ַΪ0x03��0x04����2�ֽ�
    std::vector<uint8_t> versionData = readData(id, ADDR_SOFTWARE_VERSION, LEN_SOFTWARE_VERSION);

    // ����ȡ�������Ƿ���Ч
    if (versionData.size() != 2) {
        std::cerr << "Failed to read software version." << std::endl;
        return "";
    }

    // ����ȡ������ת��Ϊ�汾�ַ���
    uint8_t majorVersion = versionData[0];
    uint8_t minorVersion = versionData[1];
    std::stringstream versionStream;
    versionStream << "v" << static_cast<int>(majorVersion) << "." << std::setw(2) << std::setfill('0') << static_cast<int>(minorVersion);

    return versionStream.str();
}

bool ServoDriver::setAngleLimits(uint8_t id, float minAngle, float maxAngle) {
    // ���ýǶ�����
    const float conversionFactor = 4096.0f / SERVO_ANGLE_RANGE; // ��ǰ����ת��ϵ��

    // �������Ƕ��Ƿ�����Ч��Χ��
    if (minAngle < -SERVO_ANGLE_HALF_RANGE || minAngle > SERVO_ANGLE_HALF_RANGE || maxAngle < -SERVO_ANGLE_HALF_RANGE || maxAngle > SERVO_ANGLE_HALF_RANGE || minAngle > maxAngle) {
        std::cerr << "Invalid angle limits." << std::endl;
        return false;
    }

    // ���Ƕ�ת��Ϊ����ڲ��ĽǶ�ֵ��0-4095��
    uint16_t minAngleInternal = static_cast<uint16_t>((minAngle + SERVO_ANGLE_HALF_RANGE) * conversionFactor);
    uint16_t maxAngleInternal = static_cast<uint16_t>((maxAngle + SERVO_ANGLE_HALF_RANGE) * conversionFactor);

    // ���ڲ��Ƕ�ֵ���Ϊ�����ֽ�
    uint8_t minAngleHigh = static_cast<uint8_t>((minAngleInternal >> 8) & 0xFF);
    uint8_t minAngleLow = static_cast<uint8_t>(minAngleInternal & 0xFF);
    uint8_t maxAngleHigh = static_cast<uint8_t>((maxAngleInternal >> 8) & 0xFF);
    uint8_t maxAngleLow = static_cast<uint8_t>(maxAngleInternal & 0xFF);

    // д����С�Ƕ�����
    std::vector<uint8_t> minAngleData = { minAngleHigh, minAngleLow };
    if (!writeData(id, ADDR_MIN_ANGLE_LIMIT, minAngleData)) {
        std::cerr << "Failed to set minimum angle limit." << std::endl;
        return false;
    }

    // д�����Ƕ�����
    std::vector<uint8_t> maxAngleData = { maxAngleHigh, maxAngleLow };
    if (!writeData(id, ADDR_MAX_ANGLE_LIMIT, maxAngleData)) {
        std::cerr << "Failed to set maximum angle limit." << std::endl;
        return false;
    }

    return true;
}

bool ServoDriver::setTargetPosition(uint8_t id, float angle) {
    // ���Ƕȷ�Χ
    if (angle < -SERVO_ANGLE_HALF_RANGE || angle > SERVO_ANGLE_HALF_RANGE) {
        std::cerr << "Angle out of range. Must be between -135 and 135 degrees." << std::endl;
        return false;
    }

    // ����ת��ϵ��
    float conversionFactor = 4096.0f / SERVO_ANGLE_RANGE;

    // ���Ƕ�ת��Ϊ�����Ҫ��0~4095��Χ�ڵ�ֵ
    uint16_t targetPosition = static_cast<uint16_t>((angle + SERVO_ANGLE_HALF_RANGE) * conversionFactor);

    // ��Ŀ��λ��ת��Ϊ�����ֽڵ�����
    uint8_t highByte = static_cast<uint8_t>(targetPosition >> 8);
    uint8_t lowByte = static_cast<uint8_t>(targetPosition & 0xFF);

    // ����дָ��
    return writeData(id, ADDR_TARGET_POSITION, { highByte, lowByte });
}

float ServoDriver::getCurrentPosition(uint8_t id) {
    // ��ȡ��ǰλ�ã���ַΪ0x38��0x39����2�ֽ�
    std::vector<uint8_t> positionData = readData(id, ADDR_CURRENT_POSITION, LEN_CURRENT_POSITION);

    // ����ȡ�������Ƿ���Ч
    if (positionData.size() != 2) {
        std::cerr << "Failed to read current position." << std::endl;
        return -999.0f; // ����һ����Чֵ
    }

    // ����ȡ�����ݺϲ�Ϊ16λ����
    uint16_t currentPosition = (static_cast<uint16_t>(positionData[0]) << 8) | positionData[1];

    // ����ת��ϵ��
    float conversionFactor = SERVO_ANGLE_RANGE / 4096.0f;

    // ����ǰλ��ת��Ϊ�Ƕ�ֵ
    float angle = (currentPosition * conversionFactor) - 135.0f;

    return angle;
}


bool ServoDriver::setMidPointAdjustment(uint8_t id, float angle) {
    // ��������λ
    if (angle < -SERVO_ANGLE_HALF_RANGE || angle > SERVO_ANGLE_HALF_RANGE) { // ���Ƕȷ�Χ
        std::cerr << "Angle out of range. Must be between -135 and 135 degrees." << std::endl;
        return false;
    }

    // ����ת��ϵ��
    float conversionFactor = 4096.0f / SERVO_ANGLE_RANGE;

    // ���Ƕ�ת��Ϊ�����Ҫ����λ����ֵ��-2048��2048��
    int16_t midPointAdjustment = static_cast<int16_t>(angle * conversionFactor);

    // ����λ����ֵת��Ϊ�����ֽڵ�����
    uint8_t highByte = static_cast<uint8_t>(midPointAdjustment >> 8);
    uint8_t lowByte = static_cast<uint8_t>(midPointAdjustment & 0xFF);

    // ����дָ��
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
        // �ȴ�һ��ʱ���ٶ�ȡ��ǰλ��
        usleep(1000); // �ȴ�1����
    }
    return true;
}
