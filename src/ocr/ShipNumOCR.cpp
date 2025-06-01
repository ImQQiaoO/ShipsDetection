#include "ShipNumOCR.h"

ShipNumOCR::ShipNumOCR() {
    ocr = new tesseract::TessBaseAPI();
    // 配置OCR参数 - 禁用一些会产生过多输出的功能
    ocr->SetVariable("tessedit_ocr_engine_mode", "3");
    ocr->SetVariable("preserve_interword_spaces", "1");
    ocr->SetVariable("tessedit_do_invert", "0");
    ocr->SetVariable("debug_file", "/dev/null");  // 禁用调试输出

    // 初始化中英文语言包
    if (ocr->Init(nullptr, "eng+chi_sim", tesseract::OEM_LSTM_ONLY)) {
        utils::utf2ansi_out << "无法初始化tesseract，尝试仅英文..." << utils::endl;
        if (ocr->Init(nullptr, "eng", tesseract::OEM_LSTM_ONLY)) {
            utils::utf2ansi_out << "tesseract初始化失败" << utils::endl;
        }
    }
}

ShipNumOCR::~ShipNumOCR() {
    if (ocr) {
        ocr->End();
        delete ocr;
    }
}

cv::Mat ShipNumOCR::preprocess_image(const cv::Mat &img) {
    cv::Mat processed;
    // 转换为灰度图
    if (img.channels() == 3) {
        cv::cvtColor(img, processed, cv::COLOR_BGR2GRAY);
    } else {
        processed = img.clone();
    }
    // 降噪
    cv::GaussianBlur(processed, processed, cv::Size(3, 3), 0);
    // 对比度增强
    cv::equalizeHist(processed, processed);
    return processed;
}

cv::Mat ShipNumOCR::getBestBinary(const cv::Mat &gray) {
    std::vector<cv::Mat> binaries;

    // 1. OTSU阈值
    cv::Mat binary1;
    cv::threshold(gray, binary1, 0, 255, cv::THRESH_BINARY + cv::THRESH_OTSU);
    binaries.push_back(binary1);

    // 2. 自适应阈值
    cv::Mat binary2;
    cv::adaptiveThreshold(gray, binary2, 255,
        cv::ADAPTIVE_THRESH_GAUSSIAN_C,
        cv::THRESH_BINARY, 21, 10);
    binaries.push_back(binary2);

    // 3. 组合阈值
    cv::Mat binary3;
    cv::bitwise_and(binary1, binary2, binary3);
    binaries.push_back(binary3);

    // 评估每个二值化图像的质量（简单评估）
    int bestIndex = 0;
    double bestScore = 0;

    for (size_t i = 0; i < binaries.size(); i++) {
        // 计算非零像素比例作为质量指标
        int nonZero = cv::countNonZero(binaries[i]);
        double ratio = static_cast<double>(nonZero) / (binaries[i].rows * binaries[i].cols);

        // 理想的文本图像应该有适中的非零像素比例（10%-50%）
        double score = 1.0 - abs(ratio - 0.3);
        if (score > bestScore) {
            bestScore = score;
            bestIndex = i;
        }
    }

    // 形态学后处理
    cv::Mat result = binaries[bestIndex].clone();
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2, 2));
    cv::morphologyEx(result, result, cv::MORPH_OPEN, kernel);  // 去噪
    cv::morphologyEx(result, result, cv::MORPH_CLOSE, kernel); // 连接

    return result;
}

std::string ShipNumOCR::recognizeText(const cv::Mat &img) const {
    utils::utf2ansi_out << "开始OCR识别..." << utils::endl;

    std::vector<std::pair<std::string, int>> results;

    // 策略1: 原图处理
    auto result1 = processImage(img, "原图");
    if (!result1.first.empty()) results.push_back(result1);

    // 策略2: 预处理后
    cv::Mat preprocessed = preprocess_image(img);
    auto result2 = processImage(preprocessed, "预处理");
    if (!result2.first.empty()) results.push_back(result2);

    // 策略3: 缩放处理
    std::vector<double> scales = {1.5, 2.0};
    for (double scale : scales) {
        cv::Mat resized;
        cv::resize(img, resized, cv::Size(), scale, scale, cv::INTER_CUBIC);
        auto result = processImage(resized, "缩放");
        if (!result.first.empty()) results.push_back(result);
    }

    return selectBestResult(results);
}

std::pair<std::string, int> ShipNumOCR::processImage(const cv::Mat &img, const std::string &method) const {
    cv::Mat gray;
    if (img.channels() == 3) {
        cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = img.clone();
    }

    // 获取最佳二值化图像
    cv::Mat binary = getBestBinary(gray);

    // 使用最有效的PSM模式
    constexpr int effective_psm_modes[] = {
        tesseract::PSM_AUTO,        // 自动模式
        tesseract::PSM_SINGLE_BLOCK, // 单块模式
        tesseract::PSM_SINGLE_LINE,  // 单行模式
    };

    std::string bestText;
    int bestConf = 0;

    for (int mode : effective_psm_modes) {
        ocr->SetPageSegMode(static_cast<tesseract::PageSegMode>(mode));
        ocr->SetImage(binary.data, binary.cols, binary.rows, 1, binary.step);

        char *text = ocr->GetUTF8Text();
        int conf = ocr->MeanTextConf();

        if (text && strlen(text) > 0 && conf > bestConf) {
            bestConf = conf;
            bestText = std::string(text);

            // 基本后处理
            bestText.erase(std::ranges::remove(bestText, '\n').begin(), bestText.end());
            bestText.erase(std::ranges::remove(bestText, '\r').begin(), bestText.end());

            // 去除前后空格
            size_t start = bestText.find_first_not_of(" \t");
            if (start != std::string::npos) {
                bestText = bestText.substr(start);
            }
            size_t end = bestText.find_last_not_of(" \t");
            if (end != std::string::npos) {
                bestText = bestText.substr(0, end + 1);
            }
        }
        delete[] text;
    }

    if (!bestText.empty() && bestConf > 30) {
        utils::utf2ansi_out << "[" << method << "] 置信度: " << bestConf
            << ", 结果: " << bestText << utils::endl;
    }

    return {bestText, bestConf};
}

std::string ShipNumOCR::selectBestResult(const std::vector<std::pair<std::string, int>> &results) {
    if (results.empty()) {
        utils::utf2ansi_out << "没有找到有效的OCR结果" << utils::endl;
        return "";
    }

    // 选择置信度最高的结果
    auto best = std::ranges::max_element(results,
                                         [](const auto &a, const auto &b) { return a.second < b.second; });

    utils::utf2ansi_out << "\n=== 最终结果 ===" << utils::endl;
    utils::utf2ansi_out << "最佳置信度: " << best->second << utils::endl;
    return best->first;
}