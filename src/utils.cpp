#include "utils.h"
#include "bytetrack.h"

std::atomic<bool> stop_inference(false); // 定义 stop_inference 并初始化为 false
std::atomic<bool> system_sleep(false);
std::atomic<bool> poweroff(false);
std::atomic<bool> is_sleeping(false); // 标志变量，表示系统是否已经进入睡眠状态

std::mutex mtx; // 互斥锁
// 舵机互斥锁
std::mutex servo_mtx;
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
	std::cerr << "Inference thread started." << std::endl;
    while (!stop_inference.load()) {
        // 从队列中获取最新的原始图像
        std::tuple<uint64_t, cv::Mat, std::vector<float>> latest_raw = image_queue.get_latest_raw();
        cv::Mat image = std::get<cv::Mat>(latest_raw);       // 获取图像
        uint64_t seq_num = std::get<uint64_t>(latest_raw);     // 获取序号
        std::vector<float> position = std::get<std::vector<float> >(latest_raw); // 获取舵机位置

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
        image_queue.push_inference(seq_num, image, json_result, od_results, position);
		new_image_available.store(true); // 设置标志变量为 true
		// std::cerr << "Inference completed for image with sequence number: " << seq_num << " index: " << ctx_index << std::endl;
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


// 函数：获取目标类别的第一个目标的中心坐标
std::pair<int, int> getTargetCenter(const object_detect_result_list& od_results) {
    for (int i = 0; i < od_results.count; ++i) {
        if (od_results.results[i].cls_id == TARGET_CLASS_ID) {
            // 提取目标框的坐标
            int left = od_results.results[i].box.left;
            int top = od_results.results[i].box.top;
            int right = od_results.results[i].box.right;
            int bottom = od_results.results[i].box.bottom;

            // 计算中心坐标
            int x = (left + right) / 2;
            int y = (top + bottom) / 2;

            // 返回中心坐标
            return std::make_pair(x, y);
        }
    }

    // 如果没有找到目标，返回 (-1, -1) 表示无效
    return std::make_pair(-1, -1);
}

// 定义相机内参矩阵的逆矩阵
const double invK[3][3] = {
    {0.00165546, 0.0, -0.5424334},
    {0.0, 0.00165294, -0.55744437},
    {0.0, 0.0, 1.0}
};

// 计算物体在舵机坐标系中的角度坐标
void calculateObjectAngles(float servoXAngle, float servoYAngle, int u, int v, float& thetaX, float& thetaY) {
    // 将像素坐标转换为归一化坐标
    double normalizedX = invK[0][0] * u + invK[0][1] * v + invK[0][2];
    double normalizedY = invK[1][0] * u + invK[1][1] * v + invK[1][2];

    // 计算物体距离主光轴在x和y方向的夹角（弧度）
    double angleX = std::atan(normalizedX);
    double angleY = std::atan(normalizedY);

    // 将弧度转换为角度
    angleX = angleX * 180.0 / M_PI;
    angleY = angleY * 180.0 / M_PI;

    // 计算物体在舵机坐标系中的角度坐标
    thetaX =  servoXAngle - angleX;
    thetaY =  servoYAngle + angleY;
}



// 这里采用简化后的单目标ByteTrack算法控制舵机
void control_servo() {
    ByteTracker tracker; // 
    // 上一步的预测值（用于无观测时作为观测值）
    double last_pred_thetaX = NAN;
    double last_pred_thetaY = NAN;

    // 平滑系数（取值范围：0~1，值越小越平滑）
    const double smoothing_factor = 0.8; // 可根据实际情况调整

    // 平滑后的角度值
    double smoothed_thetaX = NAN;
    double smoothed_thetaY = NAN;

    std::cout << "Kalman filter initialized." << std::endl;

    // 定义每个舵机的角度限制
    const double SERVO1_MIN_ANGLE = -135.0; // 舵机1的最小角度
    const double SERVO1_MAX_ANGLE = 125.0;  // 舵机1的最大角度
    const double SERVO2_MIN_ANGLE = -65; // 舵机2的最小角度
    const double SERVO2_MAX_ANGLE = 80;  // 舵机2的最大角度

    while (!stop_inference.load()) {
        // 消息通知机制
        // 等待通知，直到 new_image_available 为 true 或 stop_inference 为 true
        while (!new_image_available.load() && !stop_inference.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 避免忙等待
        }

        if (stop_inference.load()) {
            break; // 退出线程
        }
        // 重置标志
        new_image_available.store(false);
        // 获取最新的推理结果
        std::tuple<uint64_t, cv::Mat, std::string, object_detect_result_list, std::vector<float>> inference_result;
        inference_result = image_queue.get_latest_inference();
        cv::Mat image = std::get<cv::Mat>(inference_result); // 获取图像
        std::string json_result = std::get<std::string>(inference_result); // 获取 JSON 结果
        object_detect_result_list od_results = std::get<object_detect_result_list>(inference_result); // 获取检测结果
        // 获取舵机位置
        std::vector<float> position = std::get<std::vector<float>>(inference_result);
        // 获取舵机位置
        double servoXAngle = position[0];
        double servoYAngle = position[1];
        // 调用函数计算指定类别的第一个检测结果的中心坐标
        std::pair<int, int> center = getTargetCenter(od_results);
        int u = center.first;
        int v = center.second;

        float detect_thetaX = 0, detect_thetaY = 0; // 计算原始角度观测值
        bool has_detection = (u != -1 && v != -1); // 是否检测到目标

        // 跟踪逻辑核心
        if (tracker.is_active()) {
            if (has_detection) { // 如果跟踪器已激活且检测到目标
                // 计算角度观测值
                calculateObjectAngles(servoXAngle, servoYAngle, u, v, detect_thetaX, detect_thetaY);
                // 角度匹配
                if (tracker.is_match(detect_thetaX, detect_thetaY)) {
                    // 更新跟踪器状态
                    tracker.update(detect_thetaX, detect_thetaY);
                }
                else { // 角度不匹配，说明目标发生了较大位移或丢失，设定为新目标
                    tracker.initialize(detect_thetaX, detect_thetaY); // 重新初始化跟踪器
                }
            }
            else { // 没有检测到目标，使用上一步的预测值作为观测值
                if (!std::isnan(last_pred_thetaX) && !std::isnan(last_pred_thetaY)) {
                    tracker.update(last_pred_thetaX, last_pred_thetaY);
                    // 标记丢失
                    tracker.mark_loss();
                }
                else { // 上一步的预测值无效
                    // 发生错误 打印报错信息并跳过控制阶段
                    std::cerr << "Error: No valid prediction available." << std::endl;
                    // 跳过控制阶段
                    continue;
                }
            }
        }
        else if (tracker.needs_activation()) { // 如果跟踪器未激活，说明长期处于无目标状态
            if (has_detection) { // 如果检测到目标
                calculateObjectAngles(servoXAngle, servoYAngle, u, v, detect_thetaX, detect_thetaY); // 计算角度观测值
                // 激活跟踪器
                tracker.initialize(detect_thetaX, detect_thetaY); // 传入角度观测值初始化跟踪器
            }
            else { // 没有检测到目标，也没有激活跟踪器，说明系统处于休眠状态，需要跳过控制阶段
                // 跳过控制阶段
                continue;
            }
        }

        // 预测下一步状态
        std::vector<double> p = tracker.predict();
        last_pred_thetaX = p[0]; // 更新上一步的预测值
        last_pred_thetaY = p[1]; // 更新上一步的预测值

        // 平滑处理
        if (std::isnan(smoothed_thetaX) || std::isnan(smoothed_thetaY)) {
            // 如果是第一次，直接使用预测值
            smoothed_thetaX = last_pred_thetaX;
            smoothed_thetaY = last_pred_thetaY;
        }
        else {
            // 使用平滑系数对预测值进行加权平均
            smoothed_thetaX = smoothing_factor * last_pred_thetaX + (1 - smoothing_factor) * smoothed_thetaX;
            smoothed_thetaY = smoothing_factor * last_pred_thetaY + (1 - smoothing_factor) * smoothed_thetaY;
        }

        // 根据平滑后的角度值控制舵机转到指定角度
        // 舵机互斥锁保护
        std::unique_lock<std::mutex> servo_lock(servo_mtx);
        // 检查smoothed_thetaX是否在舵机1的允许范围内
        if (smoothed_thetaX >= SERVO1_MIN_ANGLE && smoothed_thetaX <= SERVO1_MAX_ANGLE) {
            servoDriver.setTargetPosition(1, smoothed_thetaX);
            usleep(10000); // 等待10毫秒
        }
        // 检查smoothed_thetaY是否在舵机2的允许范围内
        if (smoothed_thetaY >= SERVO2_MIN_ANGLE && smoothed_thetaY <= SERVO2_MAX_ANGLE) {
            servoDriver.setTargetPosition(2, smoothed_thetaY);
            usleep(10000); // 等待10毫秒
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
        // 获取最新的推理结果
        std::tuple<uint64_t, cv::Mat, std::string, object_detect_result_list, std::vector<float>> inference_result;
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

        uint64_t sequence_number = std::get<uint64_t>(inference_result);
        cv::Mat image = std::get<cv::Mat>(inference_result);
        std::string json_result = std::get<std::string>(inference_result);
        object_detect_result_list od_results = std::get<object_detect_result_list>(inference_result);

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

