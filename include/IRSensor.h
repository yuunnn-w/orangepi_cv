#include <cstdint>
#include <libuvc/libuvc.h>
#include <string>
#include <iostream>
#include <iomanip>
#include <cstring>
#include <libusb-1.0/libusb.h>
#include <vector>
#include <opencv2/opencv.hpp>
#include <chrono>
#include <ctime>

#include "threadsafequeue.h"
#include "utils.h"

class IRSensor {
public:
    // 设备信息结构体
    struct DeviceInfo {
        std::string firmwareVersion;
        std::string encoderVersion;
        std::string hardwareVersion;
        std::string deviceName;
        std::string protocolVersion;
        std::string serialNumber;
        std::string deviceID;
    };

    // 图像增强参数结构体
    struct ImageEnhancementParams {
        uint8_t channelID = 1;             // 通道号
        uint8_t noiseReduceMode = 2;       // 降噪模式 0-关闭 1-普通 2-专家
        uint8_t generalLevel = 50;         // 普通模式降噪级别 0-100
        uint8_t frameNoiseLevel = 50;      // 专家模式空域降噪 0-100
        uint8_t interFrameNoiseLevel = 50; // 专家模式时域降噪 0-100
        uint8_t paletteMode = 14;          // 伪彩色模式 1-白热 2-黑热 等
        uint8_t LSEDetailEnabled = 1;         // 细节增强使能: 0-关闭 1-开启
        uint8_t LSEDetailLevel = 100;         // 细节增强等级 0-100
    };

    // 视频调整参数结构体 
    struct VideoAdjustmentParams {
        uint8_t channelID = 1;          // 通道号
        uint8_t mirrorMode = 0;         // 镜像模式 0-关闭 1-左右 2-上下
        uint8_t corridorMode = 1;       // 走廊模式(旋转): 0-关闭 1-开启
        uint8_t digitalZoom = 0;        // 数字变倍 0-X1 1-X2 2-X4 3-X8
        uint8_t showBadPoint = 1;       // 显示坏点十字光标: 0-关闭 1-开启
        uint32_t badPointX = 500;  // 坏点X坐标，归一化0-1000（左上为原点）
        uint32_t badPointY = 500;  // 坏点Y坐标，归一化0-1000（左上为原点）
    };

    // 流配置参数结构体
    struct StreamConfig {
        uint32_t width = 256;
        uint32_t height = 192;
        uint32_t fps = 25;
        uvc_frame_format format = UVC_FRAME_FORMAT_YUYV;
    };

    IRSensor(uint16_t vid, uint16_t pid);
    ~IRSensor();

    bool initDevice();
    bool getDeviceInfo(DeviceInfo& info);
	bool configureDateTime(); // 配置设备时间
	bool configureThermalParams(); // 配置测温基本参数
	bool configureStreamType(uint8_t type); // 配置码流类型
	bool configureImageEnhancement(const ImageEnhancementParams& params); // 配置图像增强参数
	bool configureVideoAdjustment(const VideoAdjustmentParams& params); // 配置视频调整参数

    // 视频流操作方法
    bool configureStream(const StreamConfig& config);
    bool startStreaming();
    bool stopStreaming();

    void printError(int errorCode);
    // 资源释放方法
    void releaseResources();  // 释放USB接口并关闭句柄
    static void frame_callback(uvc_frame_t* frame, void* user_ptr);

    cv::Mat get_frame();


private:
    bool sendControlRequest(uint8_t bmRequestType, uint8_t bRequest,
        uint16_t wValue, uint16_t wIndex,
        uint8_t* data, uint16_t wLength);

    bool getCmdLength(uint16_t csId, uint16_t& length);
    bool switchFunction(uint8_t csId, uint8_t subFuncId);
    bool getCurrentSetting(uint8_t csId, uint8_t* data, uint16_t length);
    bool setCurrentSetting(uint8_t csId, uint8_t* data, uint16_t length);

    // libusb_context* ctx = nullptr;
    // 新增libuvc相关成员
    uvc_stream_ctrl_t ctrl;
    uvc_context_t* uvc_ctx = nullptr;
    uvc_device_t* uvc_dev = nullptr;
    uvc_device_handle_t* uvc_devh = nullptr;

    // 保留libusb设备句柄（通过uvc获取）
    libusb_device_handle* devHandle = nullptr;

    uint16_t vendorId;
    uint16_t productId;

    // 多线程安全的cv::Mat
    cv::Mat current_frame;
    std::shared_timed_mutex frame_mutex; // 读写锁，用于保护 current_frame 的访问

};