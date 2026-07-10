#ifndef WORKER_H
#define WORKER_H
#include <atomic>
#include <string>
#include <mutex>
#include <condition_variable>
#include "rga/RgaApi.h"
#include "rga/im2d.h"
#include <opencv2/opencv.hpp>
#include "dma_queue.h"
#include "rknn_api.h"
#include "yolov8-pose.h"

struct ThreadCtx {
    int           id;
    rga_buffer_t src_rga;
    rga_buffer_t dst_rga;        // DMA-BUF fd（传给 RGA/NPU）
    cv::Mat       img;           // 绑定 DMA 内存，不拥有数据
    DMABufferdst* dst_buf = nullptr; // 目标 DMA buffer 状态位
    std::mutex*   queue_mtx = nullptr; // 与 DMA 队列共用的锁，便于安全写 have/frame_id
    rknn_core_mask core_mask; // 默认使用 CORE_0
    std::atomic<bool>    idle{true};   // true=空闲，false=处理中
    std::mutex           mtx;
    std::condition_variable cv;
    std::string model_path;
    rknn_app_context_t rknn_app_ctx;
    unsigned char* dst_virt = nullptr; // 目标 DMA buffer 虚拟地址（如果需要 CPU 访问）
    bool                 wakeup = false; // 防止虚假唤醒
    bool                 stop   = false; // 退出标志
    

};

int worker(ThreadCtx& ctx);

#endif // WORKER_H
