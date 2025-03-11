// Camera.cpp
#include "camera.h"
#include <iostream>
#include "threadsafequeue.h"

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
    stop(); // ����ֹͣ��������ȷ����Դ��ȷ�ͷ�
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
        uvc_unref_device(dev); // �ͷ��豸����
        return false;
    }
    puts("Device opened");
    // uvc_print_diag(devh, stderr);
    return true;
    
}

// ����������ʵ��
bool Camera::configure_stream(uvc_frame_format format, uint32_t width, uint32_t height, uint32_t fps) {
    uvc_error_t res = uvc_get_stream_ctrl_format_size(devh, &stream_ctrl, format, width, height, fps);
    /*
     * uvc_get_stream_ctrl_format_size ������������UVC�豸����������
     * �����������£�
     *
     * devh: ָ��UVC�豸�����ָ�롣������ͨ����ͨ��uvc_open������ȡ�ġ�
     *
     * &ctrl: ָ��uvc_stream_ctrl�ṹ���ָ�롣����ṹ�����ڴ洢��������Ϣ��������ʽ���ֱ��ʡ�֡�ʵȡ�
     *
     * format: ͼ���ʽ�����õĿ�ѡ�������
     *   - UVC_FRAME_FORMAT_RGB: RGB��ʽ��ÿ�������ɺ졢�̡�������������ɡ�
     *   - UVC_FRAME_FORMAT_YUYV: YUYV��ʽ��һ�ֳ�����YUV��ʽ��ÿ��������Y��U��V����������ɡ�
     *   - UVC_FRAME_FORMAT_MJPEG: MJPEG��ʽ����Motion JPEG��ÿ��֡���Ƕ�����JPEGͼ��
     *   - UVC_FRAME_FORMAT_GRAY8: �Ҷȸ�ʽ��ÿ��������һ��8λ�ĻҶ�ֵ��ʾ��
     *   - UVC_FRAME_FORMAT_UNCOMPRESSED: δѹ����ʽ��ͨ����ԭʼ��YUV��RGB���ݡ�
     *
     * width: ͼ���ȡ������Ŀ�ѡ�������
     *   - 640: 640���ؿ�
     *   - 1280: 1280���ؿ�
     *   - 1920: 1920���ؿ�ȫ���壩��
     *   - 3840: 3840���ؿ�4K����
     *   - �����豸֧�ֵĿ��ֵ��
     *
     * height: ͼ��߶ȡ������Ŀ�ѡ�������
     *   - 480: 480���ظߡ�
     *   - 720: 720���ظߡ�
     *   - 1080: 1080���ظߣ�ȫ���壩��
     *   - 2160: 2160���ظߣ�4K����
     *   - �����豸֧�ֵĸ߶�ֵ��
     *
     * fps: ֡�ʡ������Ŀ�ѡ�������
     *   - 30: ÿ��30֡��
     *   - 60: ÿ��60֡��
     *   - 15: ÿ��15֡��
     *   - �����豸֧�ֵ�֡��ֵ��
     *
     * �ú�������һ��uvc_error_t���͵�ֵ����ʾ�����Ƿ�ɹ��������ķ���ֵ������
     *   - UVC_SUCCESS: ���óɹ���
     *   - UVC_ERROR_INVALID_PARAM: ������Ч���������豸��֧��ָ���ĸ�ʽ���ֱ��ʻ�֡�ʡ�
     *   - UVC_ERROR_IO: I/O���󣬿������豸ͨ��ʧ�ܡ�
     */
    if (res < 0) {
        uvc_perror(res, "uvc_get_stream_ctrl_format_size"); // ��ӡ����������������Ϣ
        uvc_close(devh); // �ر��豸
        uvc_unref_device(dev); // �ͷ��豸����
        uvc_exit(ctx); // �˳�UVC������
        return false;
    }
    /* Print out the result */
    // uvc_print_stream_ctrl(&stream_ctrl, stderr);

    return true;
}

// ������ʵ��
bool Camera::start() {
    uvc_error_t res = uvc_start_streaming(devh, &stream_ctrl, frame_callback, nullptr, 0); // (void*)12345
    if (res < 0) {
        uvc_perror(res, "start_streaming"); // ��ӡ������������Ϣ
        uvc_close(devh); // �ر��豸
        uvc_unref_device(dev); // �ͷ��豸����
        uvc_exit(ctx); // �˳�UVC������
        return false;
    }
    puts("Streaming...");
    return true;
}
// ֹͣ��ʵ��
void Camera::stop() {
    if (devh) {
        uvc_stop_streaming(devh);
        devh = nullptr; // ����Ϊ nullptr�������ظ��ر�
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

// �ص�����ʵ��
void Camera::frame_callback(uvc_frame_t* frame, void* user_ptr) {
    if (frame) {
        uvc_frame_t* rgb = uvc_allocate_frame(frame->width * frame->height * 3); // ����ռ�����RGB��ʽ��ͼ��
        /*
        if (!rgb) {
            printf("unable to allocate bgr frame!\n");
            return;
        }
        */
        uvc_mjpeg2rgb(frame, rgb); // ʹ�� uvc_mjpeg2rgb ת��MJPEG��RGB
        cv::Mat img(rgb->height, rgb->width, CV_8UC3, rgb->data); // ����cv::Mat����ʹ��ת�����RGB����
        image_queue.push_raw(img.clone()); // �� Mat ������У������
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