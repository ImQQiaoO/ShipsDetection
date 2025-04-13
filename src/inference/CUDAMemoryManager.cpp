#include "CUDAMemoryManager.h"
#include <cuda_runtime.h>
#include <stdexcept>
#include <iostream>


CUDAMemoryManager &CUDAMemoryManager::getInstance() {
    static CUDAMemoryManager instance;
    return instance;
}

float *CUDAMemoryManager::allocateMemory(size_t size_bytes) {
    if (gpu_data_ == nullptr || allocated_size_ < size_bytes) {
        freeMemory();

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

void CUDAMemoryManager::copyToDevice(const float *host_data, size_t size_bytes) const {
    if (gpu_data_ == nullptr) {
        throw std::runtime_error("GPU memory not allocated before copy operation");
    }

    cudaError_t cudaStatus = cudaMemcpy(gpu_data_, host_data, size_bytes, cudaMemcpyHostToDevice);
    if (cudaStatus != cudaSuccess) {
        throw std::runtime_error("Copy to device failed: " +
            std::string(cudaGetErrorString(cudaStatus)));
    }
}

void CUDAMemoryManager::freeMemory() {
    if (gpu_data_ != nullptr) {
        cudaFree(gpu_data_);
        gpu_data_ = nullptr;
        allocated_size_ = 0;
#if (!defined(NDEBUG))
        std::cout << "CUDA memory freed" << std::endl;
#endif
    }
}

CUDAMemoryManager::~CUDAMemoryManager() {
    freeMemory();
}

CUDAMemoryManager::CUDAMemoryManager() : gpu_data_(nullptr), allocated_size_(0) {}

