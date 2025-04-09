#pragma once
#include <string>
#include <vector>
#include <array>
#include <opencv2/opencv.hpp>

class DrawBoundingBox {
public:
    DrawBoundingBox();
    void draw(cv::Mat &, const std::vector<int> &, const std::vector<cv::Rect> &, const std::vector<float> &, const std::vector<int> &) const;
private:
    std::vector<std::string> class_names_;
    static const std::vector<std::array<double, 4>> box_color; // RGBA
    static std::vector<std::string> parse_class_names(const std::string &);
};
