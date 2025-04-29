// Camera.cpp
#include "camera.h"

// ���캯��ʵ��
Camera::Camera(uint16_t vid, uint16_t pid) {
    // ��ʼ��UVC������
    uvc_context_t* ctx;
    uvc_error_t res = uvc_init(&ctx, nullptr);
    if (res < 0) {
		std::cerr << "Failed to initialize UVC" << std::endl;
        uvc_perror(res, "uvc_init"); // ��ӡ��ʼ��������Ϣ
        return;
    }
    // ֱ�Ӹ����Ա����
    uvc_device_handle_t* devh;
    uvc_stream_ctrl_t ctrl;
    this->vid = vid;
    this->pid = pid;
    this->dev = nullptr; // UVC�豸
    this->devh = devh; // UVC�豸���
    this->ctx = ctx; //UVC������
    this->error = res; // ��ʼ��������
    this->stream_ctrl = ctrl; // ��ʼ�������ƽṹ��
	std::cout << "Camera object created." << std::endl;
}

// ��������ʵ��
Camera::~Camera() {
	// ���ú����ͷ���Դ
	releaseResources();
}

// ������ͷ�豸ʵ��
bool Camera::open() {
    // �����豸
    uvc_error_t res = uvc_find_device(ctx, &dev, vid, pid, nullptr);
    if (res < 0) {
		std::cerr << "Failed to find camera device." << std::endl;
        uvc_perror(res, "uvc_find_device"); // ��ӡ�����豸������Ϣ
        return false;
    }
    // ���豸
    res = uvc_open(dev, &devh);
    if (res < 0) {
		std::cerr << "Failed to open camera device." << std::endl;
        uvc_perror(res, "uvc_open"); // ��ӡ���豸������Ϣ
        return false;
    }
    puts("Device opened");
    // uvc_print_diag(devh, stderr);
    return true;
    
}

// ����������ʵ��
bool Camera::configure_stream(uvc_frame_format format, uint32_t width, uint32_t height, uint32_t fps) {
    uvc_error_t res = uvc_get_stream_ctrl_format_size(devh, &stream_ctrl, format, width, height, fps);
    if (res < 0) {
        uvc_perror(res, "uvc_get_stream_ctrl_format_size"); // ��ӡ����������������Ϣ
        return false;
    }
    // Print out the result
    // uvc_print_stream_ctrl(&stream_ctrl, stderr);

    return true;
}

// ������ʵ��
bool Camera::start() {
    uvc_error_t res = uvc_start_iso_streaming(devh, &stream_ctrl, frame_callback, this); // (void*)12345
    if (res != UVC_SUCCESS) {
        uvc_perror(res, "start_streaming"); // ��ӡ������������Ϣ
        return false;
    }
    puts("Streaming...");
    return true;
}
// ֹͣ��ʵ��
void Camera::stop() {
    if (devh) {
        uvc_stop_streaming(devh);
        puts("Streaming stopped.");
    }
}

void Camera::releaseResources() {
	if (devh) { // �ر�UVC�豸���
        // uvc_stop_streaming(devh);
        uvc_close(devh);
        devh = nullptr;
        puts("Device closed");
    }
	if (dev) { // �ͷ�UVC�豸
        uvc_unref_device(dev);
        dev = nullptr;
    }
	if (ctx) { // �ͷ�UVC������
        uvc_exit(ctx);
        ctx = nullptr;
        puts("UVC exited");
    }
}

// �ص�����ʵ��
void Camera::frame_callback(uvc_frame_t* frame, void* user_ptr) {
    Camera* camera = static_cast<Camera*>(user_ptr); // �� user_ptr ת��Ϊ Camera ���� 
    if (frame) {
        uvc_frame_t* rgb = uvc_allocate_frame(frame->width * frame->height * 3); // ����ռ�����RGB��ʽ��ͼ��
        if (!rgb) {
            printf("unable to allocate bgr frame!\n");
            return;
        }
        uvc_mjpeg2rgb(frame, rgb); // ʹ�� uvc_mjpeg2rgb ת��MJPEG��RGB
        cv::Mat img(rgb->height, rgb->width, CV_8UC3, rgb->data); // ����cv::Mat����ʹ��ת�����RGB����
        // �������ͷ����ģʽ�ǿɼ���ģʽ���ͽ�ͼ��������
        if (!camera_mode.load()) {
            image_queue.push_raw(img.clone()); // �� Mat ������У����
        }
        // ��ͼ����밲ȫ������
        // ������
		std::unique_lock<std::shared_timed_mutex> lock(camera->frame_mutex);
        camera->current_frame = img.clone(); // �����ǰ֡
		lock.unlock(); // ����
		uvc_free_frame(rgb); // �ͷ�֡
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
	cv::Mat frame = cv::Mat(); // ��ʼ���յ�cv::Mat
	// ������
	std::shared_lock<std::shared_timed_mutex> lock(frame_mutex);
	if (current_frame.empty()) {
		std::cerr << "Error: No current frame available" << std::endl;
	}
    else {
		frame = current_frame.clone(); // �����ǰ֡
    }
    lock.unlock(); // ����
	return frame; // ���ص�ǰ֡
}