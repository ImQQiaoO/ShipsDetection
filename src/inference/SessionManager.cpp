#include <iostream>
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>
#include <filesystem>
#include <vector>
#include <algorithm>
#include "SessionManager.h"

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
const wchar_t *char_to_wchar(const char *char_str) {
    if (char_str == nullptr) return nullptr;
    int size = MultiByteToWideChar(CP_UTF8, 0, char_str, -1, nullptr, 0);
    if (size == 0) return nullptr;
    wchar_t *wchar_str = new wchar_t[size];
    int result = MultiByteToWideChar(CP_UTF8, 0, char_str, -1, wchar_str, size);
    if (result == 0) {
        delete[] wchar_str;
        return nullptr;
    }
    return wchar_str;
}
#endif

SessionManager::SessionManager(const std::string &model_path) : env_(ORT_LOGGING_LEVEL_WARNING, "YOLO") {
    Ort::SessionOptions session_options;
    try {
        OrtCUDAProviderOptions cuda_options;
        cuda_options.device_id = 0;
        cuda_options.arena_extend_strategy = 0;
        cuda_options.gpu_mem_limit = SIZE_MAX;
        cuda_options.cudnn_conv_algo_search = OrtCudnnConvAlgoSearchExhaustive;
        cuda_options.do_copy_in_default_stream = 1;
        session_options.AppendExecutionProvider_CUDA(cuda_options);
        session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
    } catch (const Ort::Exception &e) {
        std::cout << "ONNX Runtime异常: " << e.what() << '\n';
        std::cout << "错误代码: " << e.GetOrtErrorCode() << '\n';
    }

#ifdef _WIN32
    session_ = std::make_unique<Ort::Session>(env_, char_to_wchar(model_path.c_str()), session_options);
#else
    session_ = std::make_unique<Ort::Session>(env, model_path.c_str(), session_options);
#endif
}

Ort::Session *SessionManager::get_session() const {
    return session_.get();
}


