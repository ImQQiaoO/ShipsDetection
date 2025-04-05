#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <array>

class draw_bounding_box {
public:
    draw_bounding_box();
    void draw(cv::Mat &, const std::vector<int> &, const std::vector<cv::Rect> &, const std::vector<float> &, const std::vector<int> &) const;
private:
    std::vector<std::string> class_names;
    static const std::vector<std::array<double, 4>> box_color; // RGBA
    static std::vector<std::string> parse_class_names(const std::string &);
};
