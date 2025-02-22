#include "utils.h"

std::atomic<bool> stop_inference(false); // ���� stop_inference ����ʼ��Ϊ false
std::atomic<bool> system_sleep(false);
std::atomic<bool> poweroff(false);
std::atomic<bool> is_sleeping(false); // ��־��������ʾϵͳ�Ƿ��Ѿ�����˯��״̬

std::mutex mtx; // ������
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
    while (!stop_inference.load()) {
        // �Ӷ����л�ȡ���µ�ԭʼͼ��
        std::pair<uint64_t, cv::Mat> latest_raw = image_queue.get_latest_raw();
        cv::Mat image = latest_raw.second;       // ��ȡͼ��
        uint64_t seq_num = latest_raw.first;     // ��ȡ���

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
        image_queue.push_inference(seq_num, image, json_result, od_results);

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


// �������Kelman�㷨���ƶ��
void control_servo(ServoDriver& servoDriver, int servo1, int servo2) {
	// ��ʼ��Kalman�˲���
    KalmanFilter kf;
    // ��ʼ��״̬���� [x, y, v_x, v_y, a_x, a_y]
    kf.init(0, 0);

    while (!stop_inference.load()) {
        // ��Ϣ֪ͨ����
		// �ȴ�֪ͨ��ֱ�� new_image_available Ϊ true �� stop_inference Ϊ true
        while (!new_image_available.load() && !stop_inference.load()) {
            // std::this_thread::yield(); // �ó� CPU ʱ��Ƭ������æ�ȴ�
            std::this_thread::sleep_for(std::chrono::milliseconds(1)); // ����æ�ȴ�
        }

        if (stop_inference.load()) {
            break; // �˳��߳�
        }
		// ���ñ�־
		new_image_available.store(false);
		// ��ȡ���µ�������
		std::tuple<uint64_t, cv::Mat, std::string, object_detect_result_list> inference_result;
        inference_result = image_queue.get_latest_inference();





        
		




    }
	

}


void show_inference() {
    //cv::namedWindow("Inference Result", cv::WINDOW_AUTOSIZE);

    std::chrono::steady_clock::time_point last_time = std::chrono::steady_clock::now();
    int frame_count = 0;
    double fps = 0.0;

    while (true) {
        // ��ȡ���µ�������
        std::tuple<uint64_t, cv::Mat, std::string, object_detect_result_list> inference_result;
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

        uint64_t sequence_number = std::get<0>(inference_result);
        cv::Mat image = std::get<1>(inference_result);
        std::string json_result = std::get<2>(inference_result);
        object_detect_result_list od_results = std::get<3>(inference_result);

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

