#pragma once

#include <tesseract/baseapi.h>
#include <opencv2/opencv.hpp>
#define NOMINMAX
#include "src/utils/Locale.hpp"

class ShipNumOCR {
    tesseract::TessBaseAPI *ocr;

public:
    ShipNumOCR();
    ~ShipNumOCR();

    // 图像预处理
    static cv::Mat preprocess_image(const cv::Mat &img);
    // 获取最佳二值化图像
    static cv::Mat getBestBinary(const cv::Mat &gray);
    // 简化的识别函数
    std::string recognizeText(const cv::Mat &img) const;

private:
    std::pair<std::string, int> processImage(const cv::Mat &img, const std::string &method) const;
    static std::string selectBestResult(const std::vector<std::pair<std::string, int>> &results);
};