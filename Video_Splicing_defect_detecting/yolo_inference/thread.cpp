#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include "worker.h"
#include "dma_queue.h"

// 引入 RGA 头文件 (根据你的系统路径调整)
#include "rga/RgaApi.h"
#include "rga/im2d.h"

// 引入你自己的 DMA 分配器头文件
#include "dma_alloc.h"

// 定义 DMA Heap 路径，根据板子实际情况修改
#define DMA_HEAP_PATH "/dev/dma_heap/system" 
// 定义线程数量，RK3588 有 8 个核心，所以我们开 8 个线程
#define NUM_THREADS 6
static const char* CoreMaskToStr(rknn_core_mask m) {
  switch (m) {
    case 0: return "CORE_0";
    case 1: return "CORE_1";
    case 2: return "CORE_2";
    case 3: return "CORE_0_1";
    case 4: return "CORE_0_1_2";
    case 5: return "AUTO";
    default: return "UNKNOWN";
  }
}

int main() {


    DMAFrameQueue thr;
    thr.init();
    
    std::vector<ThreadCtx> ctxs(NUM_THREADS);
    for (int i = 0; i < NUM_THREADS; i++) {
        ctxs[i].id = i;
        ctxs[i].src_rga = thr.src_rga[i];
        ctxs[i].dst_rga = thr.dst_rga[i];
        ctxs[i].img = thr.img_src_cpu[i];
        ctxs[i].dst_buf = thr.bufdst[i];
        ctxs[i].queue_mtx = &thr.mtx;
        ctxs[i].dst_virt = (unsigned char*)thr.bufdst[i]->data; // 直接使用目标 DMA buffer 的虚拟地址
        ctxs[i].model_path = "/root/fmc/cpp/projectwithoutyolo/yolov8_pose.rknn";
        ctxs[i].core_mask = static_cast<rknn_core_mask>(1 << (i % 3)); // 轮流分配 CORE_0, CORE_1, CORE_2
    }
    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back(worker, std::ref(ctxs[i]));
    }

    cv::VideoCapture cap;
    cap.open("/dev/video20", cv::CAP_V4L2);
    if (!cap.isOpened()) {
        cap.open("/dev/video20");
    }

    if (!cap.isOpened()) 
    {
        std::cerr << "无法打开摄像头: /dev/video20" << std::endl;
        return -1;
    }

    cap.set(cv::CAP_PROP_FRAME_WIDTH, SRC_W);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, SRC_H);

    auto fps_t0 = std::chrono::steady_clock::now();
    int fps_frames = 0;
    ThreadCtx* idle_ctx = nullptr;

    std::this_thread::sleep_for(std::chrono::seconds(1)); // 等待系统稳定
    int frame_id = 0;

    while (true) 
    {
        frame_id++;
        idle_ctx = nullptr;
        // 轮询找空闲线程（也可用条件变量通知，避免空转）
        for (auto& ctx : ctxs) {
            if (ctx.idle.load()) {
                idle_ctx = &ctx;
                idle_ctx->idle.store(false); // 立刻锁定，防止重复选中
                break;
            }
        }

        if (idle_ctx == nullptr) {
            // 所有线程都在忙，丢帧或等待
            std::this_thread::yield();
            int key = cv::waitKey(1);
            if (key == 27 || key == 'q' || key == 'Q') {
                std::cout << "收到退出按键，准备退出" << std::endl;
                break;
            }
            continue;
        }

        // 零拷贝：cap 直接写入线程的 DMA 内存
        cap >> idle_ctx->img;
        idle_ctx->dst_buf->frame_id = frame_id;
        if (idle_ctx->img.empty()) break;
        
        // 唤醒对应 worker 线程
        {
            std::lock_guard<std::mutex> lk(idle_ctx->mtx);
            idle_ctx->wakeup = true;
        }
        idle_ctx->cv.notify_one();
        dma_sync_device_to_cpu(thr.red_fd);
        thr.get_frame(thr.dst_rga, thr.red_rga);

        cv::imshow("RGA 缩放结果", thr.img_src);
        int key = cv::waitKey(1);
        if (key == 27 || key == 'q' || key == 'Q') {
            std::cout << "收到退出按键，准备退出" << std::endl;
            break;
        }

        fps_frames++;
        auto fps_t1 = std::chrono::steady_clock::now();
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(fps_t1 - fps_t0).count();
        if (elapsed_ms >= 1000) {
            double fps = fps_frames * 1000.0 / static_cast<double>(elapsed_ms);
            printf("FPS: %.2f\n", fps);
            fps_frames = 0;
            fps_t0 = fps_t1;
        }
    }

    // 5. 退出：通知所有线程停止
    for (auto& ctx : ctxs) {
        std::lock_guard<std::mutex> lk(ctx.mtx);
        ctx.stop = true;
        ctx.cv.notify_one();
    }
    for (auto& t : threads) t.join();
    cap.release();
    cv::destroyAllWindows();
    thr.free();
    return 0;
}