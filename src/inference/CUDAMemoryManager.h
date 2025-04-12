#pragma once
#include <cuda_runtime.h>
#include <stdexcept>
#include <iostream>

class CUDAMemoryManager {
public:
    // Singleton pattern to ensure only one instance exists
    static CUDAMemoryManager &getInstance() {
        static CUDAMemoryManager instance;
        return instance;
    }

    // Allocate GPU memory if not already allocated or if size changed
    float *allocateMemory(size_t size_bytes) {
        if (gpu_data_ == nullptr || allocated_size_ < size_bytes) {
            // Free previous allocation if it exists
            freeMemory();

            // Allocate new memory
            cudaError_t cudaStatus = cudaMalloc(reinterpret_cast<void **>(&gpu_data_), size_bytes);
            if (cudaStatus != cudaSuccess) {
                throw std::runtime_error("CUDA memory allocation failed: " +
                    std::string(cudaGetErrorString(cudaStatus)));
            }

            allocated_size_ = size_bytes;
#if (!defined(NDEBUG))
            std::cout << "CUDA memory allocated: " << size_bytes << " bytes" << std::endl;
#endif
        }

        return gpu_data_;
    }

    // Copy data from host to device
    void copyToDevice(const float *host_data, size_t size_bytes) {
        if (gpu_data_ == nullptr) {
            throw std::runtime_error("GPU memory not allocated before copy operation");
        }

        cudaError_t cudaStatus = cudaMemcpy(gpu_data_, host_data, size_bytes, cudaMemcpyHostToDevice);
        if (cudaStatus != cudaSuccess) {
            throw std::runtime_error("Copy to device failed: " +
                std::string(cudaGetErrorString(cudaStatus)));
        }
    }

    // Free GPU memory
    void freeMemory() {
        if (gpu_data_ != nullptr) {
            cudaFree(gpu_data_);
            gpu_data_ = nullptr;
            allocated_size_ = 0;
#if (!defined(NDEBUG))
            std::cout << "CUDA memory freed" << std::endl;
#endif
        }
    }

    // Destructor to ensure memory is freed
    ~CUDAMemoryManager() {
        freeMemory();
    }

private:
    // Private constructor for singleton
    CUDAMemoryManager() : gpu_data_(nullptr), allocated_size_(0) {}

    // Prevent copying
    CUDAMemoryManager(const CUDAMemoryManager &) = delete;
    CUDAMemoryManager &operator=(const CUDAMemoryManager &) = delete;

    float *gpu_data_;
    size_t allocated_size_;
};
