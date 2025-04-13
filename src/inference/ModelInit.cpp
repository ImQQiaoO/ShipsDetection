#include "ModelInit.h"
#include "src/utils/Locale.hpp"

const std::vector<int64_t> ModelInit::input_shape = {
    1,              /* 批处理大小 */
    3,              /* 颜色通道 */
    INPUT_WIDTH,    /* 垂直像素数 */
    INPUT_HEIGHT    /* 水平像素数 */
};

ModelInit::ModelInit(const Ort::Session *session) {
    allocator_ = std::make_unique<Ort::AllocatorWithDefaultOptions>();
    init_inference(session);
}

const std::string &ModelInit::get_input_name() {
    return input_name_;
}

const std::vector<std::string> &ModelInit::get_output_names() {
    return output_names_;
}

void ModelInit::init_inference(const Ort::Session *session) {
    // 获取输入节点名称，并保存其拷贝到 string
    auto input_name_alloc = session->GetInputNameAllocated(0, *allocator_);
    input_name_ = std::string(input_name_alloc.get());
    utils::utf2ansi_out << "输入名称: " << input_name_ << '\n';

    // 获取输出节点名称
    size_t output_count = session->GetOutputCount();
    output_names_.resize(output_count);
    for (size_t i = 0; i < output_count; i++) {
        auto output_name_alloc = session->GetOutputNameAllocated(i, *allocator_);
        output_names_[i] = std::string(output_name_alloc.get());
        utils::utf2ansi_out << "输出名称 " << i << ": " << output_names_[i] << '\n';
    }
}
