#pragma once
#include <nlohmann/json.hpp>
#include <QString>
#include <QVector>
#include <vector>
#include <iostream>
#include <regex>
#include <fstream>

struct SnapShotItem {
    std::string time;
    std::string ship_type;
    std::string image_path;
    double confidence;
    std::string identifier;
};

// SnapShotItem结构体的nlohmann::json序列化支持
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SnapShotItem, time, ship_type, image_path, confidence, identifier)

class JsonRepository {
    std::vector<SnapShotItem> item_list_;

    static std::string format_time(const std::string &image_path) {
        std::regex re(R"(capture_(\d{4})(\d{2})(\d{2})_(\d{2})(\d{2})(\d{2})_(\d{3})\.png)");
        std::smatch match;
        if (std::regex_search(image_path, match, re)) {
            return match[1].str() + "-" + match[2].str() + "-" + match[3].str() + "-" +
                match[4].str() + "-" + match[5].str() + "-" + match[6].str() + "-" +
                match[7].str();
        }
        return "";
    }

public:
    JsonRepository(const QVector<QVector<QString>> &table_contents, const QString &filename) {
        std::string curr_file_path = filename.toStdString();
        std::string curr_time = format_time(curr_file_path);
        for (const auto &table_content : table_contents) {
            std::string curr_ship_type = table_content[1].toStdString();
            double curr_confidence = table_content[2].toDouble();
            std::string curr_identifier = table_content[3].toStdString();
            item_list_.emplace_back(curr_time, curr_ship_type, curr_file_path, curr_confidence, curr_identifier);
        }

        // 输出测试
        for (auto &i : item_list_) {
            std::cout << "{\n";
            std::cout << '\t' << i.time << ", \n";
            std::cout << '\t' << i.ship_type << ", \n";
            std::cout << '\t' << i.image_path << ", \n";
            std::cout << '\t' << i.confidence << ", \n";
            std::cout << '\t' << i.identifier << "\n";
            std::cout << "}\n";
        }
    }

    void save_to_file(const std::string &json_filename) const {
        std::vector<SnapShotItem> all_items;

        // 尝试读取并反序列化现有的JSON文件
        std::ifstream ifs(json_filename);
        if (ifs.good() && ifs.peek() != std::ifstream::traits_type::eof()) {
            try {
                nlohmann::json existing_json;
                ifs >> existing_json;

                // 检查JSON是否为数组格式
                if (existing_json.is_array()) {
                    // 将JSON数组反序列化为SnapShotItem向量
                    all_items = existing_json.get<std::vector<SnapShotItem>>();
                }
            } catch (const std::exception &e) {
                // 读取或反序列化失败，输出错误信息但继续执行
                std::cerr << "Warning: Failed to read existing JSON file: " << e.what() << std::endl;
                all_items.clear(); // 确保数组A为空，重新开始
            }
        }
        ifs.close();

        // 将当前item_list_的内容添加到数组A后面
        all_items.insert(all_items.end(), item_list_.begin(), item_list_.end());

        // 将合并后的数组A序列化并写入文件
        nlohmann::json final_json = all_items;
        std::ofstream ofs(json_filename, std::ios::trunc);
        if (ofs.is_open()) {
            ofs << final_json.dump(4); // 4空格缩进
            ofs.close();
        } else {
            std::cerr << "Error: Unable to open file for writing: " << json_filename << std::endl;
        }
    }
};

