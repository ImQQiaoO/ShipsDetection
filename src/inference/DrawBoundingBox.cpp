#include <iostream>
#include <fstream>
#include <regex>
#include <string>
#include <vector>
#include "DrawBoundingBox.h"
#include "src/utils/Locale.hpp"

const std::vector<std::array<double, 4>> DrawBoundingBox::box_color = {
    // IN OPENCV IT IS BGRY
    {30, 208, 146, 255},
    {161, 164, 254, 255},
    {200, 241, 16, 255},
    {254, 33, 139, 255},
    {16, 251, 195, 255},
    {194, 232, 169, 255},
    {116, 227, 210, 255},
    {79, 68, 255, 255},
    {0, 237, 204, 255},
    {68, 243, 0, 255},
    {134, 138, 98, 255},
    {232, 208, 172, 255},
};

DrawBoundingBox::DrawBoundingBox() {
    class_names_ = parse_class_names("./ships_dataset/data.yaml");
}

void DrawBoundingBox::draw(cv::Mat &img, const std::vector<int> &indices, const std::vector<cv::Rect> &boxes, const std::vector<float> &confidences,
    const std::vector<int> &class_ids) const {
    for (int idx : indices) {
        cv::Rect box = boxes[idx];
        float conf = confidences[idx];
        int class_id = class_ids[idx];

#if (!defined(NDEBUG))
        utils::utf2ansi_out << "检测框: x=" << box.x << ", y=" << box.y
            << ", width=" << box.width << ", height=" << box.height
            << ", class=" << class_id << ", conf=" << conf << '\n';
#endif

        // 绘制框和标签
        auto curr_color = box_color[class_id % box_color.size()];
        cv::rectangle(img, box, cv::Scalar(curr_color[0], curr_color[1], curr_color[2], curr_color[3]), 2);

        // 添加标签
        std::string label = class_id < static_cast<int>(class_names_.size()) ? class_names_[class_id] : "unknown";
        label += " " + std::to_string(static_cast<int>(conf * 100)) + "%";

        int baseline = 0;
        cv::Size label_size = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseline);
        cv::rectangle(img, cv::Point(box.x, box.y - label_size.height - baseline - 10),
            cv::Point(box.x + label_size.width, box.y), cv::Scalar(curr_color[0], curr_color[1], curr_color[2], curr_color[3]), cv::FILLED);
        // 计算文本颜色
        double brightness = 0.299 * curr_color[2] + 0.587 * curr_color[1] + 0.114 * curr_color[0];
        cv::putText(img, label, cv::Point(box.x, box.y - baseline - 5),
            cv::FONT_HERSHEY_SIMPLEX, 0.5, (brightness > 186) ? cv::Scalar(0, 0, 0) : cv::Scalar(255, 255, 255));
    }
}

std::vector<std::string> DrawBoundingBox::parse_class_names(const std::string &yaml_path) {
    std::vector<std::string> class_names;
    std::ifstream file(yaml_path);

    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << yaml_path << '\n';
        return class_names;
    }

    std::string content((std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());

    std::regex names_regex(R"(names:\s*\[(.*?)\])");

    if (std::smatch names_match; std::regex_search(content, names_match, names_regex) && names_match.size() > 1) {
        std::string names_str = names_match[1].str();
        std::regex class_regex(R"(['"]([^'"]*?)['"])");
        std::string::const_iterator search_start(names_str.cbegin());
        std::smatch class_match;
        while (std::regex_search(search_start, names_str.cend(), class_match, class_regex)) {
            class_names.push_back(class_match[1].str());
            search_start = class_match.suffix().first;
        }
    }
    return class_names;
}

