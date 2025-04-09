#pragma once
#include <memory>
#include <string>
#include <vector>
#include <onnxruntime_cxx_api.h>

class ModelInit {
public:
    static constexpr int INPUT_WIDTH = 640;
    static constexpr int INPUT_HEIGHT = 640;

    explicit ModelInit(const Ort::Session *session);
    static const std::vector<int64_t> input_shape;
    const std::string &get_input_name();
    const std::vector<std::string> &get_output_names();

private:
    std::string input_name_;
    std::vector<std::string> output_names_;
    std::unique_ptr<Ort::AllocatorWithDefaultOptions> allocator_;

    void init_inference(const Ort::Session *session);
};