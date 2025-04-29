#include "IRSensor.h"

using namespace std;

IRSensor::IRSensor(uint16_t vid, uint16_t pid)
    : vendorId(vid), productId(pid) {
}

IRSensor::~IRSensor() {
    // 释放顺序很重要
    if (uvc_devh) {
        uvc_close(uvc_devh);
        uvc_devh = nullptr;
    }
    if (uvc_dev) {
        uvc_unref_device(uvc_dev);
        uvc_dev = nullptr;
    }
    if (uvc_ctx) {
        uvc_exit(uvc_ctx);
        uvc_ctx = nullptr;
    }
}

bool IRSensor::initDevice() {
    // 使用libuvc初始化
    uvc_error_t ret = uvc_init(&uvc_ctx, nullptr);
    if (ret != UVC_SUCCESS) {
        std::cerr << "uvc_init error: " << uvc_strerror(ret) << endl;
        return false;
    }

    // 查找设备
    ret = uvc_find_device(uvc_ctx, &uvc_dev, vendorId, productId, nullptr);
    if (ret != UVC_SUCCESS) {
        std::cerr << "uvc_find_device error: " << uvc_strerror(ret) << endl;
        uvc_exit(uvc_ctx);
        return false;
    }

    // 打开设备
    ret = uvc_open(uvc_dev, &uvc_devh);
    if (ret != UVC_SUCCESS) {
        std::cerr << "uvc_open error: " << uvc_strerror(ret) << endl;
        uvc_unref_device(uvc_dev);
        uvc_exit(uvc_ctx);
        return false;
    }

    // uvc_print_diag(uvc_devh, stderr); // 打印设备支持的格式和分辨率

    // 获取底层libusb句柄
    devHandle = uvc_get_libusb_handle(uvc_devh);

    // 内核驱动处理
    if (libusb_kernel_driver_active(devHandle, 0) == 1) {
        libusb_detach_kernel_driver(devHandle, 0);
    }

    // 声明接口
    int libusb_ret = libusb_claim_interface(devHandle, 0);
    if (libusb_ret < 0) {
        std::cerr << "Claim interface error: " << libusb_error_name(libusb_ret) << endl;
        uvc_close(uvc_devh);
        uvc_unref_device(uvc_dev);
        uvc_exit(uvc_ctx);
        return false;
    }

    // 验证协议版本（保持原有逻辑）
    uint8_t versionData[4] = { 0 };
    if (!sendControlRequest(0xA1, 0x81, 0x0400, 0x0A00, versionData, 4)) {
        std::cerr << "Protocol version check failed" << endl;
        return false;
    }

    if (memcmp(versionData, "2.0", 4) != 0) {
        std::cerr << "Unsupported protocol version" << endl;
        return false;
    }

    return true;
}

bool IRSensor::getDeviceInfo(DeviceInfo& info) {
    // 切换到设备信息获取功能
    if (!switchFunction(0x01, 0x01)) return false;

    uint16_t dataLen;
    if (!getCmdLength(0x0100, dataLen)) return false;

    vector<uint8_t> buffer(dataLen);
    if (!getCurrentSetting(0x01, buffer.data(), dataLen)) return false;

    info.firmwareVersion = string((char*)&buffer[0], 64);
    info.encoderVersion = string((char*)&buffer[64], 64);
    info.hardwareVersion = string((char*)&buffer[128], 64);
    info.deviceName = string((char*)&buffer[192], 64);
    info.protocolVersion = string((char*)&buffer[256], 4);
    info.serialNumber = string((char*)&buffer[260], 64);
    info.deviceID = string((char*)&buffer[420], 64);

    return true;
}

// 配置视频流参数
bool IRSensor::configureStream(const StreamConfig& config) {
    uvc_error_t ret = uvc_get_stream_ctrl_format_size(
        uvc_devh, &ctrl,
        config.format,
        config.width, config.height,
        config.fps
    );

    if (ret < 0 ) {
        std::cerr << "uvc_get_stream_ctrl_format_size failed: "
            << uvc_strerror(ret) << endl;
		std::cout << "Stream configuration failed: " << ret << std::endl;
        return false;
    }
	std::cout << "Stream configuration successful" << std::endl;
    // 打印流参数信息
    // uvc_print_stream_ctrl(&ctrl, stderr);
    return true;
}

// 启动视频流
bool IRSensor::startStreaming() {

    uvc_error_t ret = uvc_start_iso_streaming(
        uvc_devh, &ctrl,
        frame_callback,
		this // 传递当前对象指针
    );

    if (ret != UVC_SUCCESS) {
        std::cerr << "uvc_start_streaming failed: "
            << uvc_strerror(ret) << endl;
        return false;
    }

    std::cout << "Stream started successfully" << endl;
    return true;
}

// 停止视频流
bool IRSensor::stopStreaming() {
    uvc_stop_streaming(uvc_devh);
    std::cout << "Stream stopped successfully" << endl;
    return true;
}

//回调函数
void IRSensor::frame_callback(uvc_frame_t* frame, void* user_ptr) {
	IRSensor* sensor = static_cast<IRSensor*>(user_ptr); // 将 user_ptr 转换为 IRSensor 对象
    if (frame) {
        uvc_frame_t* rgb = uvc_allocate_frame(frame->width * frame->height * 3); // 分配空间用于RGB格式的图像
        uvc_yuyv2rgb(frame, rgb); // 使用 uvc_yuyv2rgb 转换YUVY到RGB
        //cv::Mat img(rgb->height, rgb->width, CV_8UC3, rgb->data); // 创建cv::Mat对象，使用转换后的RGB数据
        // 创建 256x256 的黑色画布
        cv::Mat target(256, 256, CV_8UC3, cv::Scalar(0, 0, 0));

        // 计算垂直填充偏移（上下各 32px）
        const int y_offset = 32; // (256 - 192) / 2
        const int x_offset = 0;  // 宽度已匹配，无需水平填充

        // 将原图复制到中心区域
        cv::Mat src_img(192, 256, CV_8UC3, rgb->data); // 注意高度在前：192 行 x 256 列
        cv::Rect roi(x_offset, y_offset, 256, 192);    // 目标区域的矩形范围
        src_img.copyTo(target(roi));                    // 核心拷贝操作

        if (camera_mode.load()) {
            image_queue.push_raw(target.clone()); // 深拷贝确保数据安全
        }
		// 将当前帧放入安全变量中
		std::unique_lock<std::shared_timed_mutex> lock(sensor->frame_mutex);
		sensor->current_frame = target.clone(); // 深拷贝当前帧
		lock.unlock(); // 解锁
        uvc_free_frame(rgb); // 释放帧
        return;
    }
    else
    {
        std::cerr << "Error: Received null frame" << std::endl;
        return;
    }
}

cv::Mat IRSensor::get_frame() {
	std::shared_lock<std::shared_timed_mutex> lock(frame_mutex);
	if (current_frame.empty()) {
		std::cerr << "Error: No frame available" << std::endl;
		return cv::Mat();
	}
	return current_frame.clone(); // 返回深拷贝
}

bool IRSensor::configureDateTime() {
    // 切换到校时功能
    if (!switchFunction(0x01, 0x05)) return false;

    uint16_t dataLen;
    if (!getCmdLength(0x0100, dataLen)) return false;

    // 获取当前系统时间
    auto now = chrono::system_clock::now();
    time_t now_time = chrono::system_clock::to_time_t(now);
    tm* local_time = localtime(&now_time);

    // 获取毫秒数
    auto since_epoch = now.time_since_epoch();
    auto millis = chrono::duration_cast<chrono::milliseconds>(since_epoch).count() % 1000;

    vector<uint8_t> timeData(dataLen);

    // 按协议要求填充时间数据（小端字节序）
    timeData[0] = millis & 0xFF;        // 毫秒低字节
    timeData[1] = (millis >> 8) & 0xFF; // 毫秒高字节
    timeData[2] = local_time->tm_sec;   // 秒 (0-59)
    timeData[3] = local_time->tm_min;   // 分 (0-59)
    timeData[4] = local_time->tm_hour;  // 时 (0-23)
    timeData[5] = local_time->tm_mday;  // 日 (1-31)
    timeData[6] = local_time->tm_mon + 1; // 月 (1-12)

    // 年处理（两字节小端）
    uint16_t year = local_time->tm_year + 1900;
    timeData[7] = year & 0xFF;         // 年低字节
    timeData[8] = (year >> 8) & 0xFF;  // 年高字节

    // 外部校时源使能（默认关闭）
    timeData[9] = 0x00;  // 0x00 = 关闭

    return setCurrentSetting(0x01, timeData.data(), dataLen);
}

bool IRSensor::configureThermalParams() {
    // 切换到测温基本参数配置
    if (!switchFunction(0x03, 0x01)) return false;

    uint16_t dataLen;
    if (!getCmdLength(0x0300, dataLen)) return false;

    vector<uint8_t> params(dataLen);
    // 设置默认参数
    params[1] = 1;   // 开启测温功能
    params[2] = 1;   // 显示最高温
    params[5] = 1;   // 温度单位（摄氏度）
    params[6] = 2;   // 测温范围（-20~150℃）
    params[31] = 2;  // 码流叠加温度信息

    return setCurrentSetting(0x03, params.data(), dataLen);
}

bool IRSensor::configureStreamType(uint8_t type) {
    // 切换到实时码流配置
    if (!switchFunction(0x03, 0x05)) return false;

    uint16_t dataLen;
    if (!getCmdLength(0x0300, dataLen)) return false;

    vector<uint8_t> streamConfig(dataLen);
    streamConfig[1] = type;  // 设置码流类型

    return setCurrentSetting(0x03, streamConfig.data(), dataLen);
}

// 核心控制请求发送函数
bool IRSensor::sendControlRequest(uint8_t bmRequestType, uint8_t bRequest,
    uint16_t wValue, uint16_t wIndex,
    uint8_t* data, uint16_t wLength)
{
    int transferred = 0;
    int ret = libusb_control_transfer(devHandle,
        bmRequestType,
        bRequest,
        wValue,
        wIndex,
        data,
        wLength,
        1000);  // 超时1秒

    if (ret < 0) {
        std::cerr << "Control transfer failed: " << libusb_error_name(ret) << endl;
        return false;
    }
    return true;
}
// 配置图像增强参数
bool IRSensor::configureImageEnhancement(const ImageEnhancementParams& params) {
    // 参数验证（扩展所有参数范围）
    if (params.noiseReduceMode > 2 ||          // 0-关闭,1-普通模式,2-专家模式
        params.generalLevel > 100 ||           // 普通模式降噪级别0-100
        params.frameNoiseLevel > 100 ||        // 专家模式空域降噪级别0-100
        params.interFrameNoiseLevel > 100 ||   // 专家模式时域降噪级别0-100
        params.paletteMode > 22 ||             // 伪彩色模式范围1-22（文档定义）
        params.LSEDetailEnabled > 1 ||         // 细节增强使能0-关闭,1-开启
        params.LSEDetailLevel > 100) {         // 细节增强等级0-100
        std::cerr << "Invalid image enhancement parameters" << endl;
        return false;
    }

    // 切换到图像管理功能（CS_ID=0x02）和图像增强子功能（Sub-function ID=0x05）
    if (!switchFunction(0x02, 0x05)) return false;

    // 获取参数长度（根据文档A.2.5节，完整参数长度为8字节）
    uint16_t dataLen;
    if (!getCmdLength(0x0200, dataLen) || dataLen < 8) {  // wValue=0x0205（CS_ID=0x02, Sub-function=0x05）
        std::cerr << "Failed to get valid data length" << endl;
        return false;
    }
    // 打印参数长度
	std::cout << "Data length: " << dataLen << endl;

    // 构造数据包
    vector<uint8_t> buffer(dataLen, 0); // 初始化填充0
    buffer[0] = params.channelID;       // Offset 0: 通道号（固定为1）
    buffer[1] = params.noiseReduceMode; // Offset 1: 降噪模式
    buffer[2] = params.generalLevel;    // Offset 2: 普通模式降噪级别
    buffer[3] = params.frameNoiseLevel; // Offset 3: 专家模式空域降噪级别
    buffer[4] = params.interFrameNoiseLevel; // Offset 4: 专家模式时域降噪级别
    buffer[5] = params.paletteMode;     // Offset 5: 伪彩色模式
    buffer[6] = params.LSEDetailEnabled;// Offset 6: 细节增强使能（0/1）
    buffer[7] = params.LSEDetailLevel;  // Offset 7: 细节增强等级

    // 发送SET_CUR请求（CS_ID=0x02）
    return setCurrentSetting(0x02, buffer.data(), dataLen);
}


// 配置视频调整参数
bool IRSensor::configureVideoAdjustment(const VideoAdjustmentParams& params) {
    // 参数验证（扩展检查项）
    if (params.mirrorMode > 2 ||  // 镜像模式: 0-关闭 1-左右 2-上下
        params.digitalZoom > 3 || // 数字变倍: 0~3
        params.badPointX > 1000 || params.badPointY > 1000 ||
        params.corridorMode > 1 || params.showBadPoint > 1) {
        std::cerr << "Invalid video adjustment parameters" << endl;
        return false;
    }

    // 切换到图像管理功能（CS_ID=0x02）和视频调整子功能（Sub-function ID=0x06）
    if (!switchFunction(0x02, 0x06)) return false;

    // 获取参数长度（根据文档A.2.6节，完整参数长度为28字节）
    uint16_t dataLen;
    if (!getCmdLength(0x0200, dataLen) || dataLen < 28) {
        std::cerr << "Failed to get valid data length" << endl;
        return false;
    }
    // 打印参数长度
	std::cout << "\nData length: " << dataLen << endl;
    // 构造数据包（严格按文档偏移填充）
    vector<uint8_t> buffer(dataLen, 0); // 初始化填充0
    // --- 基础参数 ---
    buffer[0] = params.channelID;       // Offset 0: 通道号
    buffer[1] = params.mirrorMode;      // Offset 1: 镜像模式
    buffer[2] = 0x00;                   // Offset 2: 视频制式固定为1-PAL
    buffer[3] = params.corridorMode;    // Offset 3: 走廊模式
    buffer[4] = params.digitalZoom;     // Offset 4: 数字变倍
    // Offset 5-13: 9字节保留，已初始化为0

    // --- 坏点光标参数 ---
    buffer[14] = params.showBadPoint;   // Offset14: 显示坏点光标
    buffer[15] = 0x00;                  // Offset15: 移动方式默认直接下发坐标（未实现移动指令）

    // 写入归一化坐标（小端4字节）
    // badPointX @ Offset16-19
    buffer[16] = (params.badPointX >> 0) & 0xFF;  // LSB
    buffer[17] = (params.badPointX >> 8) & 0xFF;
    buffer[18] = (params.badPointX >> 16) & 0xFF;
    buffer[19] = (params.badPointX >> 24) & 0xFF; // MSB

    // badPointY @ Offset20-23
    buffer[20] = (params.badPointY >> 0) & 0xFF;  // LSB
    buffer[21] = (params.badPointY >> 8) & 0xFF;
    buffer[22] = (params.badPointY >> 16) & 0xFF;
    buffer[23] = (params.badPointY >> 24) & 0xFF; // MSB

    // Offset24-27: 其他保留字段（已初始化为0）

    // 发送SET_CUR请求
    return setCurrentSetting(0x02, buffer.data(), dataLen);
}


bool IRSensor::getCmdLength(uint16_t csId, uint16_t& length) {
    uint8_t buffer[2] = { 0 };
    if (!sendControlRequest(0xA1, 0x85, csId, 0x0A00, buffer, 2)) {
        return false;
    }
    length = (buffer[1] << 8) | buffer[0];  // 小端转换
    return true;
}

bool IRSensor::switchFunction(uint8_t csId, uint8_t subFuncId) {
    uint8_t functionData[2] = { csId, subFuncId };
    return sendControlRequest(0x21, 0x01, 0x0500, 0x0A00, functionData, 2);
}

bool IRSensor::getCurrentSetting(uint8_t csId, uint8_t* data, uint16_t length) {
    return sendControlRequest(0xA1, 0x81, csId << 8, 0x0A00, data, length);
}

bool IRSensor::setCurrentSetting(uint8_t csId, uint8_t* data, uint16_t length) {
    return sendControlRequest(0x21, 0x01, csId << 8, 0x0A00, data, length);
}

void IRSensor::printError(int errorCode) {
    const char* errors[] = {
        "Normal",
        "Device busy",
        "Invalid state",
        "Insufficient power",
        "Parameter out of range",
        "Invalid Unit ID",
        "Invalid CS ID",
        "Invalid request type",
        "Invalid parameter value",
        "Invalid sub-function",
        "Execution exception",
        "Protocol error",
        "Big data transfer error",
        "Length error"
    };

    if (errorCode >= 0 && errorCode <= 0x0D) {
        std::cerr << "Error code 0x" << hex << setw(2) << setfill('0') << errorCode
            << ": " << errors[errorCode] << endl;
    }
    else {
        std::cerr << "Unknown error code: 0x" << hex << errorCode << endl;
    }
}

void IRSensor::releaseResources() {
    if (uvc_devh) {
        uvc_close(uvc_devh);
        uvc_devh = nullptr;
    }
    if (uvc_dev) {
        uvc_unref_device(uvc_dev);
        uvc_dev = nullptr;
    }
    if (uvc_ctx) {
        uvc_exit(uvc_ctx);
        uvc_ctx = nullptr;
    }
}