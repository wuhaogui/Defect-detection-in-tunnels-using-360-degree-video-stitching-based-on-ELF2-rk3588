#ifndef DMA_QUEUE_H
#define DMA_QUEUE_H

#include "rga/RgaApi.h"
#include "rga/im2d.h"
#include <mutex>
#include "dma_alloc.h"
#include <opencv2/opencv.hpp>

#define DMA_HEAP_PATH "/dev/dma_heap/system"

constexpr int SRC_W = 640;
constexpr int SRC_H = 480;
constexpr int DST_W = 640;
constexpr int DST_H = 640;
constexpr int DMA_QUEUE_CAPACITY = 6;

struct DMABuffersrc {
    int frame_id;
    bool sign;
    int have;
    char data[SRC_W * SRC_H * 3];
};

struct DMABufferdst {
    int frame_id;
    int have;
    bool sign;
    char data[DST_W * DST_H * 3];
};

struct DMAFrameQueue 
{
    int buf_num = DMA_QUEUE_CAPACITY;       // 缓冲区数量
    std::mutex mtx;        // 队列锁
    
    int src_fd[DMA_QUEUE_CAPACITY] = {-1};
    int dst_fd[DMA_QUEUE_CAPACITY] = {-1};
    int red_fd = -1;
    void* src_virt[DMA_QUEUE_CAPACITY] = {nullptr};
    void* dst_virt[DMA_QUEUE_CAPACITY] = {nullptr};
    void* red_virt = nullptr;
    DMABuffersrc *bufsrc[DMA_QUEUE_CAPACITY];
    DMABufferdst *bufdst[DMA_QUEUE_CAPACITY];
    rga_buffer_t src_rga[DMA_QUEUE_CAPACITY];
    rga_buffer_t dst_rga[DMA_QUEUE_CAPACITY];
    rga_buffer_t red_rga;
    cv::Mat img_src_cpu[DMA_QUEUE_CAPACITY];
    cv::Mat img_src;

    void init();
    void get_frame(rga_buffer_t dst_rga[DMA_QUEUE_CAPACITY], rga_buffer_t red_rga);
    void free();
};

#endif // DMA_QUEUE_H
