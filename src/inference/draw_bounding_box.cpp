#include <iostream>
#include <fstream>
#include <regex>
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include "draw_bounding_box.h"

draw_bounding_box::draw_bounding_box() {
    class_names = parse_class_names("./ships_dataset/data.yaml");
}

void draw_bounding_box::draw(cv::Mat &img, const std::vector<int>& indices, const std::vector<cv::Rect> &boxes , const std::vector<float> &confidences,
                             const std::vector<int> &class_ids) const {
    for (int idx : indices) {
        cv::Rect box = boxes[idx];
        float conf = confidences[idx];
        int class_id = class_ids[idx];

        std::cout << "检测框: x=" << box.x << ", y=" << box.y
            << ", width=" << box.width << ", height=" << box.height
            << ", class=" << class_id << ", conf=" << conf << '\n';

        // 绘制框和标签
        cv::rectangle(img, box, cv::Scalar(0, 255, 0), 2);

        // 添加标签
        std::string label = class_id < class_names.size() ? class_names[class_id] : "unknown";
        label += " " + std::to_string(static_cast<int>(conf * 100)) + "%";

        int baseline = 0;
        cv::Size label_size = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseline);
        cv::rectangle(img, cv::Point(box.x, box.y - label_size.height - baseline - 10),
            cv::Point(box.x + label_size.width, box.y), cv::Scalar(0, 255, 0), cv::FILLED);
        cv::putText(img, label, cv::Point(box.x, box.y - baseline - 5),
            cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0));
    }
}

std::vector<std::string> draw_bounding_box::parse_class_names(const std::string &yaml_path) {
    std::vector<std::string> classNames;
    std::ifstream file(yaml_path);

    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << yaml_path << '\n';
        return classNames;
    }

    std::string content((std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());

    std::regex namesRegex(R"(names:\s*\[(.*?)\])");
    std::smatch namesMatch;

    if (std::regex_search(content, namesMatch, namesRegex) && namesMatch.size() > 1) {
        std::string namesStr = namesMatch[1].str();
        std::regex classRegex(R"(['"]([^'"]*?)['"])");
        std::string::const_iterator searchStart(namesStr.cbegin());
        std::smatch classMatch;
        while (std::regex_search(searchStart, namesStr.cend(), classMatch, classRegex)) {
            classNames.push_back(classMatch[1].str());
            searchStart = classMatch.suffix().first;
        }
    }
    return classNames;
}

