// Camera.cpp
#include "camera.h"

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
	// 调用函数释放资源
	releaseResources();
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
        return false;
    }
    puts("Device opened");
    // uvc_print_diag(devh, stderr);
    return true;
    
}

// 配置流参数实现
bool Camera::configure_stream(uvc_frame_format format, uint32_t width, uint32_t height, uint32_t fps) {
    uvc_error_t res = uvc_get_stream_ctrl_format_size(devh, &stream_ctrl, format, width, height, fps);
    if (res < 0) {
        uvc_perror(res, "uvc_get_stream_ctrl_format_size"); // 打印配置流参数错误信息
        return false;
    }
    // Print out the result
    // uvc_print_stream_ctrl(&stream_ctrl, stderr);

    return true;
}

// 启动流实现
bool Camera::start() {
    uvc_error_t res = uvc_start_iso_streaming(devh, &stream_ctrl, frame_callback, this); // (void*)12345
    if (res != UVC_SUCCESS) {
        uvc_perror(res, "start_streaming"); // 打印启动流错误信息
        return false;
    }
    puts("Streaming...");
    return true;
}
// 停止流实现
void Camera::stop() {
    if (devh) {
        uvc_stop_streaming(devh);
        puts("Streaming stopped.");
    }
}

void Camera::releaseResources() {
	if (devh) { // 关闭UVC设备句柄
        // uvc_stop_streaming(devh);
        uvc_close(devh);
        devh = nullptr;
        puts("Device closed");
    }
	if (dev) { // 释放UVC设备
        uvc_unref_device(dev);
        dev = nullptr;
    }
	if (ctx) { // 释放UVC上下文
        uvc_exit(ctx);
        ctx = nullptr;
        puts("UVC exited");
    }
}

// 回调函数实现
void Camera::frame_callback(uvc_frame_t* frame, void* user_ptr) {
    Camera* camera = static_cast<Camera*>(user_ptr); // 将 user_ptr 转换为 Camera 对象 
    if (frame) {
        uvc_frame_t* rgb = uvc_allocate_frame(frame->width * frame->height * 3); // 分配空间用于RGB格式的图像
        if (!rgb) {
            printf("unable to allocate bgr frame!\n");
            return;
        }
        uvc_mjpeg2rgb(frame, rgb); // 使用 uvc_mjpeg2rgb 转换MJPEG到RGB
        cv::Mat img(rgb->height, rgb->width, CV_8UC3, rgb->data); // 创建cv::Mat对象，使用转换后的RGB数据
        // 如果摄像头工作模式是可见光模式，就将图像加入队列
        if (!camera_mode.load()) {
            image_queue.push_raw(img.clone()); // 将 Mat 加入队列，深拷贝
        }
        // 把图像放入安全变量中
        // 锁保护
		std::unique_lock<std::shared_timed_mutex> lock(camera->frame_mutex);
        camera->current_frame = img.clone(); // 深拷贝当前帧
		lock.unlock(); // 解锁
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

cv::Mat Camera::get_frame() {
	cv::Mat frame = cv::Mat(); // 初始化空的cv::Mat
	// 锁保护
	std::shared_lock<std::shared_timed_mutex> lock(frame_mutex);
	if (current_frame.empty()) {
		std::cerr << "Error: No current frame available" << std::endl;
	}
    else {
		frame = current_frame.clone(); // 深拷贝当前帧
    }
    lock.unlock(); // 解锁
	return frame; // 返回当前帧
}