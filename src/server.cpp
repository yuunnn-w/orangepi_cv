// Server.cpp
#include "server.h"

cv::Mat draw_detection_boxes(const cv::Mat& image, const object_detect_result_list& od_results) {
    // 创建图像副本
    cv::Mat output_image = image.clone();

    // 遍历所有检测结果
    for (int i = 0; i < od_results.count; ++i) {
        const object_detect_result& result = od_results.results[i];

        // 绘制边界框
        cv::Rect bbox(
            result.box.left,
            result.box.top,
            result.box.right - result.box.left,
            result.box.bottom - result.box.top
        );
        cv::rectangle(output_image, bbox, cv::Scalar(0, 255, 0), 2);

        // 准备标签文本
        std::ostringstream label;
        label << "ID: " << result.cls_id
            << " Conf: " << std::fixed << std::setprecision(2) << result.prop;

        // 获取文本尺寸
        int baseLine = 0;
        cv::Size label_size = cv::getTextSize(
            label.str(),
            cv::FONT_HERSHEY_SIMPLEX,
            0.5,  // 字体大小
            1,    // 字体粗细
            &baseLine
        );

        // 绘制文本背景
        cv::rectangle(
            output_image,
            cv::Point(result.box.left, result.box.top - label_size.height),
            cv::Point(result.box.left + label_size.width, result.box.top + baseLine),
            cv::Scalar(0, 255, 0),
            cv::FILLED
        );

        // 绘制文本
        cv::putText(
            output_image,
            label.str(),
            cv::Point(result.box.left, result.box.top),
            cv::FONT_HERSHEY_SIMPLEX,
            0.5,
            cv::Scalar(0, 0, 0),  // 黑色文本
            1,
            cv::LINE_AA
        );
    }

    return output_image;
}

