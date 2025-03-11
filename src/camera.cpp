// Camera.cpp
#include "camera.h"
#include <iostream>
#include "threadsafequeue.h"

// 构造函数实现
Camera::Camera(uint16_t vid, uint16_t pid) {
    // 初始化UVC上下文
    uvc_context_t* ctx;
    uvc_error_t res = uvc_init(&ctx, nullptr);
    if (res < 0) {
		std::cerr << "Failed to initialize UVC" << std::endl;
        uvc_perror(res, "uvc_init"); // 打印初始化错误信息
        return;
    }
    // 直接赋予成员属性
    uvc_device_handle_t* devh;
    uvc_stream_ctrl_t ctrl;
    this->vid = vid;
    this->pid = pid;
    this->dev = nullptr; // UVC设备
    this->devh = devh; // UVC设备句柄
    this->ctx = ctx; //UVC上下文
    this->error = res; // 初始化错误码
    this->stream_ctrl = ctrl; // 初始化流控制结构体
	std::cout << "Camera object created." << std::endl;
}

// 析构函数实现
Camera::~Camera() {
    stop(); // 调用停止流方法，确保资源正确释放
}

// 打开摄像头设备实现
bool Camera::open() {
    // 查找设备
    uvc_error_t res = uvc_find_device(ctx, &dev, vid, pid, nullptr);
    if (res < 0) {
		std::cerr << "Failed to find camera device." << std::endl;
        uvc_perror(res, "uvc_find_device"); // 打印查找设备错误信息
        return false;
    }
    // 打开设备
    res = uvc_open(dev, &devh);
    if (res < 0) {
		std::cerr << "Failed to open camera device." << std::endl;
        uvc_perror(res, "uvc_open"); // 打印打开设备错误信息
        uvc_unref_device(dev); // 释放设备引用
        return false;
    }
    puts("Device opened");
    // uvc_print_diag(devh, stderr);
    return true;
    
}

// 配置流参数实现
bool Camera::configure_stream(uvc_frame_format format, uint32_t width, uint32_t height, uint32_t fps) {
    uvc_error_t res = uvc_get_stream_ctrl_format_size(devh, &stream_ctrl, format, width, height, fps);
    /*
     * uvc_get_stream_ctrl_format_size 函数用于配置UVC设备的流参数。
     * 参数解释如下：
     *
     * devh: 指向UVC设备句柄的指针。这个句柄通常是通过uvc_open函数获取的。
     *
     * &ctrl: 指向uvc_stream_ctrl结构体的指针。这个结构体用于存储流控制信息，包括格式、分辨率、帧率等。
     *
     * format: 图像格式。常用的可选项包括：
     *   - UVC_FRAME_FORMAT_RGB: RGB格式，每个像素由红、绿、蓝三个分量组成。
     *   - UVC_FRAME_FORMAT_YUYV: YUYV格式，一种常见的YUV格式，每个像素由Y、U、V三个分量组成。
     *   - UVC_FRAME_FORMAT_MJPEG: MJPEG格式，即Motion JPEG，每个帧都是独立的JPEG图像。
     *   - UVC_FRAME_FORMAT_GRAY8: 灰度格式，每个像素由一个8位的灰度值表示。
     *   - UVC_FRAME_FORMAT_UNCOMPRESSED: 未压缩格式，通常是原始的YUV或RGB数据。
     *
     * width: 图像宽度。常见的可选项包括：
     *   - 640: 640像素宽。
     *   - 1280: 1280像素宽。
     *   - 1920: 1920像素宽（全高清）。
     *   - 3840: 3840像素宽（4K）。
     *   - 其他设备支持的宽度值。
     *
     * height: 图像高度。常见的可选项包括：
     *   - 480: 480像素高。
     *   - 720: 720像素高。
     *   - 1080: 1080像素高（全高清）。
     *   - 2160: 2160像素高（4K）。
     *   - 其他设备支持的高度值。
     *
     * fps: 帧率。常见的可选项包括：
     *   - 30: 每秒30帧。
     *   - 60: 每秒60帧。
     *   - 15: 每秒15帧。
     *   - 其他设备支持的帧率值。
     *
     * 该函数返回一个uvc_error_t类型的值，表示配置是否成功。常见的返回值包括：
     *   - UVC_SUCCESS: 配置成功。
     *   - UVC_ERROR_INVALID_PARAM: 参数无效，可能是设备不支持指定的格式、分辨率或帧率。
     *   - UVC_ERROR_IO: I/O错误，可能是设备通信失败。
     */
    if (res < 0) {
        uvc_perror(res, "uvc_get_stream_ctrl_format_size"); // 打印配置流参数错误信息
        uvc_close(devh); // 关闭设备
        uvc_unref_device(dev); // 释放设备引用
        uvc_exit(ctx); // 退出UVC上下文
        return false;
    }
    /* Print out the result */
    // uvc_print_stream_ctrl(&stream_ctrl, stderr);

    return true;
}

// 启动流实现
bool Camera::start() {
    uvc_error_t res = uvc_start_streaming(devh, &stream_ctrl, frame_callback, nullptr, 0); // (void*)12345
    if (res < 0) {
        uvc_perror(res, "start_streaming"); // 打印启动流错误信息
        uvc_close(devh); // 关闭设备
        uvc_unref_device(dev); // 释放设备引用
        uvc_exit(ctx); // 退出UVC上下文
        return false;
    }
    puts("Streaming...");
    return true;
}
// 停止流实现
void Camera::stop() {
    if (devh) {
        uvc_stop_streaming(devh);
        devh = nullptr; // 设置为 nullptr，避免重复关闭
        puts("Done streaming.");
    }
    if (devh) {
        uvc_close(devh);
        devh = nullptr;
        puts("Device closed");
    }
    if (dev) {
        uvc_unref_device(dev);
        dev = nullptr;
    }
    if (ctx) {
        uvc_exit(ctx);
        ctx = nullptr;
        puts("UVC exited");
    }
}

// 回调函数实现
void Camera::frame_callback(uvc_frame_t* frame, void* user_ptr) {
    if (frame) {
        uvc_frame_t* rgb = uvc_allocate_frame(frame->width * frame->height * 3); // 分配空间用于RGB格式的图像
        /*
        if (!rgb) {
            printf("unable to allocate bgr frame!\n");
            return;
        }
        */
        uvc_mjpeg2rgb(frame, rgb); // 使用 uvc_mjpeg2rgb 转换MJPEG到RGB
        cv::Mat img(rgb->height, rgb->width, CV_8UC3, rgb->data); // 创建cv::Mat对象，使用转换后的RGB数据
        image_queue.push_raw(img.clone()); // 将 Mat 加入队列，深拷贝！
		uvc_free_frame(rgb); // 释放帧
        // std::cerr << "Push img successfully!" << std::endl;
        return;
    }
    else
    {
        std::cerr << "Error: Received null frame" << std::endl;
        return;
    }
}