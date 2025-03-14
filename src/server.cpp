// Server.cpp
#include "server.h"

cv::Mat draw_detection_boxes(const cv::Mat& image, const object_detect_result_list& od_results) {
    // ����ͼ�񸱱�
    cv::Mat output_image = image.clone();

    // �������м����
    for (int i = 0; i < od_results.count; ++i) {
        const object_detect_result& result = od_results.results[i];

        // ���Ʊ߽��
        cv::Rect bbox(
            result.box.left,
            result.box.top,
            result.box.right - result.box.left,
            result.box.bottom - result.box.top
        );
        cv::rectangle(output_image, bbox, cv::Scalar(0, 255, 0), 2);

        // ׼����ǩ�ı�
        std::ostringstream label;
        label << "ID: " << result.cls_id
            << " Conf: " << std::fixed << std::setprecision(2) << result.prop;

        // ��ȡ�ı��ߴ�
        int baseLine = 0;
        cv::Size label_size = cv::getTextSize(
            label.str(),
            cv::FONT_HERSHEY_SIMPLEX,
            0.5,  // �����С
            1,    // �����ϸ
            &baseLine
        );

        // �����ı�����
        cv::rectangle(
            output_image,
            cv::Point(result.box.left, result.box.top - label_size.height),
            cv::Point(result.box.left + label_size.width, result.box.top + baseLine),
            cv::Scalar(0, 255, 0),
            cv::FILLED
        );

        // �����ı�
        cv::putText(
            output_image,
            label.str(),
            cv::Point(result.box.left, result.box.top),
            cv::FONT_HERSHEY_SIMPLEX,
            0.5,
            cv::Scalar(0, 0, 0),  // ��ɫ�ı�
            1,
            cv::LINE_AA
        );
    }

    return output_image;
}

