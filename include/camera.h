// camera.h
#ifndef CAMERA_H
#define CAMERA_H

#include <libuvc/libuvc.h>
#include <opencv2/opencv.hpp>

class Camera {
public:
    Camera(uint16_t vid, uint16_t pid);
    ~Camera();

    bool open();
    bool configure_stream(uvc_frame_format format, uint32_t width, uint32_t height, uint32_t fps);
    bool start();
    void stop();

private:
    static void frame_callback(uvc_frame_t* frame, void* user_ptr);

    uvc_context_t* ctx;
    uvc_device_t* dev;
    uvc_device_handle_t* devh;
    uvc_stream_ctrl_t stream_ctrl;
    uvc_error_t error;
    uint16_t vid;
    uint16_t pid;
};

#endif // CAMERA_H