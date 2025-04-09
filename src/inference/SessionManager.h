#pragma once
#include <iostream>
#include <onnxruntime_cxx_api.h>
#include <string>

#ifdef _WIN32
const wchar_t *char_to_wchar(const char *char_str);
#endif

class SessionManager {
public:
    explicit SessionManager(const std::string &model_path);
    Ort::Session *get_session() const;
private:
    Ort::Env env_;
    std::unique_ptr<Ort::Session> session_;
};
