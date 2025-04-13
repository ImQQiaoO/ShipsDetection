#pragma once

class CUDAMemoryManager {
public:
    static CUDAMemoryManager &getInstance();

    float *allocateMemory(size_t size_bytes);

    void copyToDevice(const float *host_data, size_t size_bytes) const;

    void freeMemory();

    ~CUDAMemoryManager();

    // Prevent copying
    CUDAMemoryManager(const CUDAMemoryManager &) = delete;
    CUDAMemoryManager &operator=(const CUDAMemoryManager &) = delete;

private:
    // Private constructor for singleton
    CUDAMemoryManager();

    float *gpu_data_;
    size_t allocated_size_;
};
