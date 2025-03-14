#include "utils.h"
#include "bytetrack.h"

std::atomic<bool> stop_inference(false); // ���� stop_inference ����ʼ��Ϊ false
std::atomic<bool> system_sleep(false);
std::atomic<bool> poweroff(false);
std::atomic<bool> is_sleeping(false); // ��־��������ʾϵͳ�Ƿ��Ѿ�����˯��״̬

std::mutex mtx; // ������
// ���������
std::mutex servo_mtx;
std::atomic<bool> new_image_available(false); // ��־��������ʾ�Ƿ�����ͼ��


// �� object_detect_result_list ת��Ϊ JSON �ַ���
std::string convert_to_json(const object_detect_result_list& result_list) {
    json j;

    // ��� id �� count
    j["id"] = result_list.id;
    j["count"] = result_list.count;

    // ���ÿ�������
    json results_array = json::array();
    for (int i = 0; i < result_list.count; ++i) {
        const auto& result = result_list.results[i];
        json result_json;
        result_json["cls_id"] = result.cls_id;
        result_json["prop"] = result.prop;
        result_json["box"]["left"] = result.box.left;
        result_json["box"]["top"] = result.box.top;
        result_json["box"]["right"] = result.box.right;
        result_json["box"]["bottom"] = result.box.bottom;
        results_array.push_back(result_json);
    }

    j["results"] = results_array;

    // ת��Ϊ�ַ���
    return j.dump();
}

// ������
void inference_images(rknn_model& model, int ctx_index) {
	std::cerr << "Inference thread started." << std::endl;
    while (!stop_inference.load()) {
        // �Ӷ����л�ȡ���µ�ԭʼͼ��
        std::tuple<uint64_t, cv::Mat, std::vector<float>> latest_raw = image_queue.get_latest_raw();
        cv::Mat image = std::get<cv::Mat>(latest_raw);       // ��ȡͼ��
        uint64_t seq_num = std::get<uint64_t>(latest_raw);     // ��ȡ���
        std::vector<float> position = std::get<std::vector<float> >(latest_raw); // ��ȡ���λ��

        if (image.empty()) {
            // std::this_thread::yield();
            std::this_thread::sleep_for(std::chrono::milliseconds(1)); // ����æ�ȴ�
            continue;
        }

        // ��������
        object_detect_result_list od_results;
        int ret = model.run_inference(image, ctx_index, &od_results);
        if (ret < 0) {
            std::cerr << "rknn_run fail! ret=" << ret << std::endl;
            continue;
        }

        // ��������ת��Ϊ JSON �ַ���
        std::string json_result = convert_to_json(od_results);

        // ���������������
        image_queue.push_inference(seq_num, image, json_result, od_results, position);
		new_image_available.store(true); // ���ñ�־����Ϊ true
		// std::cerr << "Inference completed for image with sequence number: " << seq_num << " index: " << ctx_index << std::endl;
        // ��ӡ��־
        //std::cout << "Inference completed for image with sequence number: " << seq_num << " index: " << ctx_index <<std::endl;
        //std::cout << "JSON result: " << json_result << std::endl;
    }

    std::cout << "Inference thread stopped." << std::endl;
}

/*
// ������
void inference_images(rknn_model* model, int ctx_index) {
    int frame_count = 0; // ֡��������
    auto start_time = std::chrono::high_resolution_clock::now(); // ��ʼʱ��

    while (!stop_inference) {
        auto step_start_time = std::chrono::high_resolution_clock::now(); // ���迪ʼʱ��

        // �Ӷ����л�ȡ���µ�ԭʼͼ��
        std::pair<uint64_t, cv::Mat> latest_raw = image_queue.get_latest_raw();
        cv::Mat image = latest_raw.second;       // ��ȡͼ��
        uint64_t seq_num = latest_raw.first;     // ��ȡ���

        auto get_image_time = std::chrono::high_resolution_clock::now(); // ��ȡͼ�����ʱ��

        if (image.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5)); // ����æ�ȴ�
            continue;
        }

        // ��������
        object_detect_result_list od_results;
        int ret = model->run_inference(image, ctx_index, &od_results);
        auto inference_time = std::chrono::high_resolution_clock::now(); // �������ʱ��

        if (ret < 0) {
            std::cerr << "rknn_run fail! ret=" << ret << std::endl;
            continue;
        }

        // ��������ת��Ϊ JSON �ַ���
        std::string json_result = convert_to_json(od_results);
        auto convert_time = std::chrono::high_resolution_clock::now(); // ת������ʱ��

        // ���������������
        image_queue.push_inference(seq_num, image, json_result, od_results);
        auto push_result_time = std::chrono::high_resolution_clock::now(); // ������н���ʱ��

        // ����֡��
        frame_count++;

        // ����֡��
        auto current_time = std::chrono::high_resolution_clock::now();
        auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count() / 1000.0; // ת��Ϊ��

        if (elapsed_time >= 1.0) { // ÿ1�����һ��֡��
            double fps = frame_count / elapsed_time; // ����֡��
            std::cout << "FPS: " << fps << std::endl; // ��ӡ֡��

            // ����֡����ʱ��
            frame_count = 0;
            start_time = std::chrono::high_resolution_clock::now();
        }

        // ����ÿ������ĺ�ʱ
        auto total_time = std::chrono::duration_cast<std::chrono::microseconds>(push_result_time - step_start_time).count() / 1000000.0;
        auto get_image_duration = std::chrono::duration_cast<std::chrono::microseconds>(get_image_time - step_start_time).count() / 1000000.0;
        auto inference_duration = std::chrono::duration_cast<std::chrono::microseconds>(inference_time - get_image_time).count() / 1000000.0;
        auto convert_duration = std::chrono::duration_cast<std::chrono::microseconds>(convert_time - inference_time).count() / 1000000.0;
        auto push_result_duration = std::chrono::duration_cast<std::chrono::microseconds>(push_result_time - convert_time).count() / 1000000.0;

        // ��ӡÿ������ĺ�ʱ����ٷֱ�
        std::cout << std::fixed << std::setprecision(6);
        std::cout << "Get Image Time: " << get_image_duration << "s (" << (get_image_duration / total_time) * 100 << "%)" << std::endl;
        std::cout << "Inference Time: " << inference_duration << "s (" << (inference_duration / total_time) * 100 << "%)" << std::endl;
        std::cout << "Convert Time: " << convert_duration << "s (" << (convert_duration / total_time) * 100 << "%)" << std::endl;
        std::cout << "Push Result Time: " << push_result_duration << "s (" << (push_result_duration / total_time) * 100 << "%)" << std::endl;
        std::cout << "Total Time: " << total_time << "s" << std::endl;
    }

    std::cout << "Inference thread stopped." << std::endl;
}
*/


// ��������ȡĿ�����ĵ�һ��Ŀ�����������
std::pair<int, int> getTargetCenter(const object_detect_result_list& od_results) {
    for (int i = 0; i < od_results.count; ++i) {
        if (od_results.results[i].cls_id == TARGET_CLASS_ID) {
            // ��ȡĿ��������
            int left = od_results.results[i].box.left;
            int top = od_results.results[i].box.top;
            int right = od_results.results[i].box.right;
            int bottom = od_results.results[i].box.bottom;

            // ������������
            int x = (left + right) / 2;
            int y = (top + bottom) / 2;

            // ������������
            return std::make_pair(x, y);
        }
    }

    // ���û���ҵ�Ŀ�꣬���� (-1, -1) ��ʾ��Ч
    return std::make_pair(-1, -1);
}

// ��������ڲξ���������
const double invK[3][3] = {
    {0.00165546, 0.0, -0.5424334},
    {0.0, 0.00165294, -0.55744437},
    {0.0, 0.0, 1.0}
};

// ���������ڶ������ϵ�еĽǶ�����
void calculateObjectAngles(float servoXAngle, float servoYAngle, int u, int v, float& thetaX, float& thetaY) {
    // ����������ת��Ϊ��һ������
    double normalizedX = invK[0][0] * u + invK[0][1] * v + invK[0][2];
    double normalizedY = invK[1][0] * u + invK[1][1] * v + invK[1][2];

    // �������������������x��y����ļнǣ����ȣ�
    double angleX = std::atan(normalizedX);
    double angleY = std::atan(normalizedY);

    // ������ת��Ϊ�Ƕ�
    angleX = angleX * 180.0 / M_PI;
    angleY = angleY * 180.0 / M_PI;

    // ���������ڶ������ϵ�еĽǶ�����
    thetaX =  servoXAngle - angleX;
    thetaY =  servoYAngle + angleY;
}



// ������ü򻯺�ĵ�Ŀ��ByteTrack�㷨���ƶ��
void control_servo() {
    ByteTracker tracker; // 
    // ��һ����Ԥ��ֵ�������޹۲�ʱ��Ϊ�۲�ֵ��
    double last_pred_thetaX = NAN;
    double last_pred_thetaY = NAN;

    // ƽ��ϵ����ȡֵ��Χ��0~1��ֵԽСԽƽ����
    const double smoothing_factor = 0.8; // �ɸ���ʵ���������

    // ƽ����ĽǶ�ֵ
    double smoothed_thetaX = NAN;
    double smoothed_thetaY = NAN;

    std::cout << "Kalman filter initialized." << std::endl;

    // ����ÿ������ĽǶ�����
    const double SERVO1_MIN_ANGLE = -135.0; // ���1����С�Ƕ�
    const double SERVO1_MAX_ANGLE = 125.0;  // ���1�����Ƕ�
    const double SERVO2_MIN_ANGLE = -65; // ���2����С�Ƕ�
    const double SERVO2_MAX_ANGLE = 80;  // ���2�����Ƕ�

    while (!stop_inference.load()) {
        // ��Ϣ֪ͨ����
        // �ȴ�֪ͨ��ֱ�� new_image_available Ϊ true �� stop_inference Ϊ true
        while (!new_image_available.load() && !stop_inference.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // ����æ�ȴ�
        }

        if (stop_inference.load()) {
            break; // �˳��߳�
        }
        // ���ñ�־
        new_image_available.store(false);
        // ��ȡ���µ�������
        std::tuple<uint64_t, cv::Mat, std::string, object_detect_result_list, std::vector<float>> inference_result;
        inference_result = image_queue.get_latest_inference();
        cv::Mat image = std::get<cv::Mat>(inference_result); // ��ȡͼ��
        std::string json_result = std::get<std::string>(inference_result); // ��ȡ JSON ���
        object_detect_result_list od_results = std::get<object_detect_result_list>(inference_result); // ��ȡ�����
        // ��ȡ���λ��
        std::vector<float> position = std::get<std::vector<float>>(inference_result);
        // ��ȡ���λ��
        double servoXAngle = position[0];
        double servoYAngle = position[1];
        // ���ú�������ָ�����ĵ�һ�����������������
        std::pair<int, int> center = getTargetCenter(od_results);
        int u = center.first;
        int v = center.second;

        float detect_thetaX = 0, detect_thetaY = 0; // ����ԭʼ�Ƕȹ۲�ֵ
        bool has_detection = (u != -1 && v != -1); // �Ƿ��⵽Ŀ��

        // �����߼�����
        if (tracker.is_active()) {
            if (has_detection) { // ����������Ѽ����Ҽ�⵽Ŀ��
                // ����Ƕȹ۲�ֵ
                calculateObjectAngles(servoXAngle, servoYAngle, u, v, detect_thetaX, detect_thetaY);
                // �Ƕ�ƥ��
                if (tracker.is_match(detect_thetaX, detect_thetaY)) {
                    // ���¸�����״̬
                    tracker.update(detect_thetaX, detect_thetaY);
                }
                else { // �ǶȲ�ƥ�䣬˵��Ŀ�귢���˽ϴ�λ�ƻ�ʧ���趨Ϊ��Ŀ��
                    tracker.initialize(detect_thetaX, detect_thetaY); // ���³�ʼ��������
                }
            }
            else { // û�м�⵽Ŀ�꣬ʹ����һ����Ԥ��ֵ��Ϊ�۲�ֵ
                if (!std::isnan(last_pred_thetaX) && !std::isnan(last_pred_thetaY)) {
                    tracker.update(last_pred_thetaX, last_pred_thetaY);
                    // ��Ƕ�ʧ
                    tracker.mark_loss();
                }
                else { // ��һ����Ԥ��ֵ��Ч
                    // �������� ��ӡ������Ϣ���������ƽ׶�
                    std::cerr << "Error: No valid prediction available." << std::endl;
                    // �������ƽ׶�
                    continue;
                }
            }
        }
        else if (tracker.needs_activation()) { // ���������δ���˵�����ڴ�����Ŀ��״̬
            if (has_detection) { // �����⵽Ŀ��
                calculateObjectAngles(servoXAngle, servoYAngle, u, v, detect_thetaX, detect_thetaY); // ����Ƕȹ۲�ֵ
                // ���������
                tracker.initialize(detect_thetaX, detect_thetaY); // ����Ƕȹ۲�ֵ��ʼ��������
            }
            else { // û�м�⵽Ŀ�꣬Ҳû�м����������˵��ϵͳ��������״̬����Ҫ�������ƽ׶�
                // �������ƽ׶�
                continue;
            }
        }

        // Ԥ����һ��״̬
        std::vector<double> p = tracker.predict();
        last_pred_thetaX = p[0]; // ������һ����Ԥ��ֵ
        last_pred_thetaY = p[1]; // ������һ����Ԥ��ֵ

        // ƽ������
        if (std::isnan(smoothed_thetaX) || std::isnan(smoothed_thetaY)) {
            // ����ǵ�һ�Σ�ֱ��ʹ��Ԥ��ֵ
            smoothed_thetaX = last_pred_thetaX;
            smoothed_thetaY = last_pred_thetaY;
        }
        else {
            // ʹ��ƽ��ϵ����Ԥ��ֵ���м�Ȩƽ��
            smoothed_thetaX = smoothing_factor * last_pred_thetaX + (1 - smoothing_factor) * smoothed_thetaX;
            smoothed_thetaY = smoothing_factor * last_pred_thetaY + (1 - smoothing_factor) * smoothed_thetaY;
        }

        // ����ƽ����ĽǶ�ֵ���ƶ��ת��ָ���Ƕ�
        // �������������
        std::unique_lock<std::mutex> servo_lock(servo_mtx);
        // ���smoothed_thetaX�Ƿ��ڶ��1������Χ��
        if (smoothed_thetaX >= SERVO1_MIN_ANGLE && smoothed_thetaX <= SERVO1_MAX_ANGLE) {
            servoDriver.setTargetPosition(1, smoothed_thetaX);
            usleep(10000); // �ȴ�10����
        }
        // ���smoothed_thetaY�Ƿ��ڶ��2������Χ��
        if (smoothed_thetaY >= SERVO2_MIN_ANGLE && smoothed_thetaY <= SERVO2_MAX_ANGLE) {
            servoDriver.setTargetPosition(2, smoothed_thetaY);
            usleep(10000); // �ȴ�10����
        }
        servo_lock.unlock();
    }
}



void show_inference() {
    //cv::namedWindow("Inference Result", cv::WINDOW_AUTOSIZE);

    std::chrono::steady_clock::time_point last_time = std::chrono::steady_clock::now();
    int frame_count = 0;
    double fps = 0.0;

    while (true) {
        // ��ȡ���µ�������
        std::tuple<uint64_t, cv::Mat, std::string, object_detect_result_list, std::vector<float>> inference_result;
        bool valid_result = true;

        // ��ѯֱ����ȡ����Ч�ļ����
        while (valid_result) {
            inference_result = image_queue.get_latest_inference();
            if (!std::get<1>(inference_result).empty()) { // ����Ƿ�Ϊ��Ч���
                valid_result = false;
            }
            else {
                std::this_thread::sleep_for(std::chrono::milliseconds(1)); // ����æ�ȴ�
            }
        }

        uint64_t sequence_number = std::get<uint64_t>(inference_result);
        cv::Mat image = std::get<cv::Mat>(inference_result);
        std::string json_result = std::get<std::string>(inference_result);
        object_detect_result_list od_results = std::get<object_detect_result_list>(inference_result);

        // ����һ�� RGB ��ʽ�ĸ������ڻ���
        cv::Mat image_rgb = image.clone();
        cv::cvtColor(image_rgb, image_rgb, cv::COLOR_RGB2BGR); // ת���� BGR ��ʽ�Ա���ʾ��ȷ��ɫ

        // ���Ƽ���
        for (int i = 0; i < od_results.count; ++i) {
            object_detect_result result = od_results.results[i];

            // ���ƾ��ο�
            cv::Rect rect(result.box.left, result.box.top, result.box.right - result.box.left, result.box.bottom - result.box.top);
            cv::rectangle(image_rgb, rect, cv::Scalar(0, 255, 0), 2); // ��ɫ����

            // ����ı���ǩ
            std::ostringstream label;
            label << "ID: " << result.cls_id << " Conf: " << std::fixed << std::setprecision(2) << result.prop;
            int baseLine = 0;
            cv::Size label_size = cv::getTextSize(label.str(), cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);
            cv::rectangle(image_rgb, cv::Point(result.box.left, result.box.top - label_size.height),
                cv::Point(result.box.left + label_size.width, result.box.top + baseLine),
                cv::Scalar(0, 255, 0), -1); // ��䱳��
            cv::putText(image_rgb, label.str(), cv::Point(result.box.left, result.box.top),
                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1, cv::LINE_AA); // ��ɫ����
        }

        // ����֡��
        std::chrono::steady_clock::time_point current_time = std::chrono::steady_clock::now();
        frame_count++;
        if (std::chrono::duration_cast<std::chrono::seconds>(current_time - last_time).count() >= 1) {
            fps = frame_count / std::chrono::duration<double>(current_time - last_time).count();
            frame_count = 0;
            last_time = current_time;
        }

        // �ڻ������Ͻ���ʾ֡��
        std::ostringstream fps_label;
        fps_label << "FPS: " << std::fixed << std::setprecision(2) << fps;
        cv::putText(image_rgb, fps_label.str(), cv::Point(image_rgb.cols - 150, 30),
            cv::FONT_HERSHEY_SIMPLEX, 0.75, cv::Scalar(0, 255, 0), 2, cv::LINE_AA);

        // ��ʾͼ��
        //cv::imshow("Inference Result", image_rgb);

        // �ȴ��������룬�ӳ� 1 ����
        if (cv::waitKey(1) == 27) { // ���� ESC ���˳�
            break;
        }
    }

    //cv::destroyAllWindows();
}

