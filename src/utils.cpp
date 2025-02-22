#include "utils.h"

std::atomic<bool> stop_inference(false); // 定义 stop_inference 并初始化为 false
std::atomic<bool> system_sleep(false);
std::atomic<bool> poweroff(false);
std::atomic<bool> is_sleeping(false); // 标志变量，表示系统是否已经进入睡眠状态

std::mutex mtx; // 互斥锁
std::atomic<bool> new_image_available(false); // 标志变量，表示是否有新图像

// 将 object_detect_result_list 转换为 JSON 字符串
std::string convert_to_json(const object_detect_result_list& result_list) {
    json j;

    // 添加 id 和 count
    j["id"] = result_list.id;
    j["count"] = result_list.count;

    // 添加每个检测结果
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

    // 转换为字符串
    return j.dump();
}

// 推理函数
void inference_images(rknn_model& model, int ctx_index) {
    while (!stop_inference.load()) {
        // 从队列中获取最新的原始图像
        std::pair<uint64_t, cv::Mat> latest_raw = image_queue.get_latest_raw();
        cv::Mat image = latest_raw.second;       // 获取图像
        uint64_t seq_num = latest_raw.first;     // 获取序号

        if (image.empty()) {
            // std::this_thread::yield();
            std::this_thread::sleep_for(std::chrono::milliseconds(1)); // 避免忙等待
            continue;
        }

        // 进行推理
        object_detect_result_list od_results;
        int ret = model.run_inference(image, ctx_index, &od_results);
        if (ret < 0) {
            std::cerr << "rknn_run fail! ret=" << ret << std::endl;
            continue;
        }

        // 将推理结果转换为 JSON 字符串
        std::string json_result = convert_to_json(od_results);

        // 将推理结果存入队列
        image_queue.push_inference(seq_num, image, json_result, od_results);

        // 打印日志
        //std::cout << "Inference completed for image with sequence number: " << seq_num << " index: " << ctx_index <<std::endl;
        //std::cout << "JSON result: " << json_result << std::endl;
    }

    std::cout << "Inference thread stopped." << std::endl;
}

/*
// 推理函数
void inference_images(rknn_model* model, int ctx_index) {
    int frame_count = 0; // 帧数计数器
    auto start_time = std::chrono::high_resolution_clock::now(); // 开始时间

    while (!stop_inference) {
        auto step_start_time = std::chrono::high_resolution_clock::now(); // 步骤开始时间

        // 从队列中获取最新的原始图像
        std::pair<uint64_t, cv::Mat> latest_raw = image_queue.get_latest_raw();
        cv::Mat image = latest_raw.second;       // 获取图像
        uint64_t seq_num = latest_raw.first;     // 获取序号

        auto get_image_time = std::chrono::high_resolution_clock::now(); // 获取图像结束时间

        if (image.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5)); // 避免忙等待
            continue;
        }

        // 进行推理
        object_detect_result_list od_results;
        int ret = model->run_inference(image, ctx_index, &od_results);
        auto inference_time = std::chrono::high_resolution_clock::now(); // 推理结束时间

        if (ret < 0) {
            std::cerr << "rknn_run fail! ret=" << ret << std::endl;
            continue;
        }

        // 将推理结果转换为 JSON 字符串
        std::string json_result = convert_to_json(od_results);
        auto convert_time = std::chrono::high_resolution_clock::now(); // 转换结束时间

        // 将推理结果存入队列
        image_queue.push_inference(seq_num, image, json_result, od_results);
        auto push_result_time = std::chrono::high_resolution_clock::now(); // 存入队列结束时间

        // 更新帧数
        frame_count++;

        // 计算帧率
        auto current_time = std::chrono::high_resolution_clock::now();
        auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count() / 1000.0; // 转换为秒

        if (elapsed_time >= 1.0) { // 每1秒计算一次帧率
            double fps = frame_count / elapsed_time; // 计算帧率
            std::cout << "FPS: " << fps << std::endl; // 打印帧率

            // 重置帧数和时间
            frame_count = 0;
            start_time = std::chrono::high_resolution_clock::now();
        }

        // 计算每个步骤的耗时
        auto total_time = std::chrono::duration_cast<std::chrono::microseconds>(push_result_time - step_start_time).count() / 1000000.0;
        auto get_image_duration = std::chrono::duration_cast<std::chrono::microseconds>(get_image_time - step_start_time).count() / 1000000.0;
        auto inference_duration = std::chrono::duration_cast<std::chrono::microseconds>(inference_time - get_image_time).count() / 1000000.0;
        auto convert_duration = std::chrono::duration_cast<std::chrono::microseconds>(convert_time - inference_time).count() / 1000000.0;
        auto push_result_duration = std::chrono::duration_cast<std::chrono::microseconds>(push_result_time - convert_time).count() / 1000000.0;

        // 打印每个步骤的耗时及其百分比
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


// 这里采用Kelman算法控制舵机
void control_servo(ServoDriver& servoDriver, int servo1, int servo2) {
	// 初始化Kalman滤波器
    KalmanFilter kf;
    // 初始化状态向量 [x, y, v_x, v_y, a_x, a_y]
    kf.init(0, 0);

    while (!stop_inference.load()) {
        // 消息通知机制
		// 等待通知，直到 new_image_available 为 true 或 stop_inference 为 true
        while (!new_image_available.load() && !stop_inference.load()) {
            // std::this_thread::yield(); // 让出 CPU 时间片，避免忙等待
            std::this_thread::sleep_for(std::chrono::milliseconds(1)); // 避免忙等待
        }

        if (stop_inference.load()) {
            break; // 退出线程
        }
		// 重置标志
		new_image_available.store(false);
		// 获取最新的推理结果
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
        // 获取最新的推理结果
        std::tuple<uint64_t, cv::Mat, std::string, object_detect_result_list> inference_result;
        bool valid_result = true;

        // 轮询直到获取到有效的检测结果
        while (valid_result) {
            inference_result = image_queue.get_latest_inference();
            if (!std::get<1>(inference_result).empty()) { // 检查是否为无效结果
                valid_result = false;
            }
            else {
                std::this_thread::sleep_for(std::chrono::milliseconds(1)); // 避免忙等待
            }
        }

        uint64_t sequence_number = std::get<0>(inference_result);
        cv::Mat image = std::get<1>(inference_result);
        std::string json_result = std::get<2>(inference_result);
        object_detect_result_list od_results = std::get<3>(inference_result);

        // 创建一个 RGB 格式的副本用于绘制
        cv::Mat image_rgb = image.clone();
        cv::cvtColor(image_rgb, image_rgb, cv::COLOR_RGB2BGR); // 转换回 BGR 格式以便显示正确颜色

        // 绘制检测框
        for (int i = 0; i < od_results.count; ++i) {
            object_detect_result result = od_results.results[i];

            // 绘制矩形框
            cv::Rect rect(result.box.left, result.box.top, result.box.right - result.box.left, result.box.bottom - result.box.top);
            cv::rectangle(image_rgb, rect, cv::Scalar(0, 255, 0), 2); // 绿色框线

            // 添加文本标签
            std::ostringstream label;
            label << "ID: " << result.cls_id << " Conf: " << std::fixed << std::setprecision(2) << result.prop;
            int baseLine = 0;
            cv::Size label_size = cv::getTextSize(label.str(), cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);
            cv::rectangle(image_rgb, cv::Point(result.box.left, result.box.top - label_size.height),
                cv::Point(result.box.left + label_size.width, result.box.top + baseLine),
                cv::Scalar(0, 255, 0), -1); // 填充背景
            cv::putText(image_rgb, label.str(), cv::Point(result.box.left, result.box.top),
                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1, cv::LINE_AA); // 黑色文字
        }

        // 计算帧率
        std::chrono::steady_clock::time_point current_time = std::chrono::steady_clock::now();
        frame_count++;
        if (std::chrono::duration_cast<std::chrono::seconds>(current_time - last_time).count() >= 1) {
            fps = frame_count / std::chrono::duration<double>(current_time - last_time).count();
            frame_count = 0;
            last_time = current_time;
        }

        // 在画面右上角显示帧率
        std::ostringstream fps_label;
        fps_label << "FPS: " << std::fixed << std::setprecision(2) << fps;
        cv::putText(image_rgb, fps_label.str(), cv::Point(image_rgb.cols - 150, 30),
            cv::FONT_HERSHEY_SIMPLEX, 0.75, cv::Scalar(0, 255, 0), 2, cv::LINE_AA);

        // 显示图像
        //cv::imshow("Inference Result", image_rgb);

        // 等待按键输入，延迟 1 毫秒
        if (cv::waitKey(1) == 27) { // 按下 ESC 键退出
            break;
        }
    }

    //cv::destroyAllWindows();
}

