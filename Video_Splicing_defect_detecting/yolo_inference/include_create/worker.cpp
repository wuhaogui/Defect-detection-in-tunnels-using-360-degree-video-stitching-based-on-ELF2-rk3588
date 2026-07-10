#include <iostream>
#include <vector>
#include <opencv2/opencv.hpp>
#include "worker.h"

// 引入 RGA 头文件 (根据你的系统路径调整)
#include "rga/RgaApi.h"
#include "rga/im2d.h"

// 引入你自己的 DMA 分配器头文件
#include "dma_alloc.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "yolov8-pose.h"
#include "image_drawing.h"
// 定义 DMA Heap 路径，根据板子实际情况修改
#define DMA_HEAP_PATH "/dev/dma_heap/system" 
// 或者 "/dev/dma_heap/rga"

int worker(ThreadCtx& ctx) 
{
    int skeleton[38] ={16, 14, 14, 12, 17, 15, 15, 13, 12, 13, 6, 12, 7, 13, 6, 7, 6, 8, 
            7, 9, 8, 10, 9, 11, 2, 3, 1, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7}; 

    int ret;
    init_post_process();
    ret = init_yolov8_pose_model(ctx.model_path.c_str(), &ctx.rknn_app_ctx, ctx.core_mask);
    if (ret != 0)
    {
        printf("init_yolov8_pose_model fail! ret=%d model_path=%s\n", ret, ctx.model_path.c_str());
        goto out;
    }
    object_detect_result_list od_results;
    
    image_buffer_t src_image;
    memset(&src_image, 0, sizeof(image_buffer_t));
    src_image.width = 640;
    src_image.height = 640;
    src_image.format = IMAGE_FORMAT_RGB888;
    src_image.virt_addr = ctx.dst_virt; // 直接使用目标 DMA buffer 的虚拟地址
    src_image.width_stride = src_image.width * 3;
    src_image.height_stride = src_image.height;
    src_image.size = src_image.width_stride * src_image.height;
    src_image.fd = ctx.dst_rga.fd; // 直接使用目标 DMA buffer 的 fd
    
    char text[256];
    

    while (true)
    {  
        // 等待被唤醒或停止
        std::unique_lock<std::mutex> lk(ctx.mtx);
        ctx.cv.wait(lk, [&ctx]{ return ctx.wakeup || ctx.stop; });
        if (ctx.stop) break;
        ctx.wakeup = false;
        lk.unlock();
        dma_sync_cpu_to_device(ctx.src_rga.fd);
        IM_STATUS status = imresize(ctx.src_rga, ctx.dst_rga);
        if (status != IM_STATUS_SUCCESS) 
        {
            std::cerr << "RGA 缩放失败: " << status << std::endl;
            ctx.idle.store(true);
            continue;
        }
        // printf("线程名称%d: RGA 缩放成功!\n", ctx.id);
        ret = inference_yolov8_pose_model(&ctx.rknn_app_ctx, &src_image, &od_results);
        if (ret != 0)
        {
            printf("inference_yolov8_pose_model fail! ret=%d\n", ret);
            goto out;
        }
    
        for (int i = 0; i < od_results.count; i++)
        {
            object_detect_result *det_result = &(od_results.results[i]);
            // printf("%s @ (%d %d %d %d) %.3f\n", coco_cls_to_name(det_result->cls_id),
            //     det_result->box.left, det_result->box.top,
            //     det_result->box.right, det_result->box.bottom,
            //     det_result->prop);
            int x1 = det_result->box.left;
            int y1 = det_result->box.top;
            int x2 = det_result->box.right;
            int y2 = det_result->box.bottom;

            draw_rectangle(&src_image, x1, y1, x2 - x1, y2 - y1, COLOR_BLUE, 3);

            sprintf(text, "%s %.1f%%", coco_cls_to_name(det_result->cls_id), det_result->prop * 100);
            draw_text(&src_image, text, x1, y1 - 20, COLOR_RED, 10);

            for (int j = 0; j < 38/2; ++j)
            {
                draw_line(&src_image, (int)(det_result->keypoints[skeleton[2*j]-1][0]),(int)(det_result->keypoints[skeleton[2*j]-1][1]),
                (int)(det_result->keypoints[skeleton[2*j+1]-1][0]),(int)(det_result->keypoints[skeleton[2*j+1]-1][1]),COLOR_ORANGE,3);
            }
            
            for (int j = 0; j < 17; ++j)
            {
                draw_circle(&src_image, (int)(det_result->keypoints[j][0]),(int)(det_result->keypoints[j][1]),1, COLOR_YELLOW,1);
            }
        }

        // printf("线程名称%d: RGA 缩放成功!\n", ctx.id);
        // 标记该帧可读
        if (ctx.dst_buf && ctx.queue_mtx) {
            std::lock_guard<std::mutex> qlk(*ctx.queue_mtx);
            ctx.dst_buf->frame_id++;
            ctx.dst_buf->have = 1;
        }

        // 处理完成，标记空闲
        ctx.idle.store(true);

    }
    
out:
    deinit_post_process();
    ret = release_yolov8_pose_model(&ctx.rknn_app_ctx);
    if (ret != 0)
    {
        printf("release_yolov8_pose_model fail! ret=%d\n", ret);
    }
    return 0;

}