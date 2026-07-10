#include "dma_queue.h"
#include <stdio.h>

void DMAFrameQueue::init() 
{
    for(int i=0; i<buf_num; i++)
    {
        dma_buf_alloc(DMA_HEAP_PATH,sizeof(DMABuffersrc),&src_fd[i], &src_virt[i]);
        dma_buf_alloc(DMA_HEAP_PATH,sizeof(DMABufferdst),&dst_fd[i], &dst_virt[i]);
            // stride 与 width/height 保持一致，分辨率与分配大小匹配
            src_rga[i] = wrapbuffer_fd(src_fd[i], SRC_W, SRC_H, RK_FORMAT_BGR_888, SRC_W, SRC_H);
            dst_rga[i] = wrapbuffer_fd(dst_fd[i], DST_W, DST_H, RK_FORMAT_RGB_888, DST_W, DST_H);
        bufsrc[i] = (DMABuffersrc *)src_virt[i];
        bufdst[i] = (DMABufferdst *)dst_virt[i];
            img_src_cpu[i] = cv::Mat(SRC_H, SRC_W, CV_8UC3, bufsrc[i]->data);
        bufsrc[i]->have = 0;
        bufdst[i]->have = 0;
        bufsrc[i]->frame_id = 0;
        bufdst[i]->frame_id = 0;
        
    }
        dma_buf_alloc(DMA_HEAP_PATH, sizeof(DMABufferdst), &red_fd, &red_virt);
        red_rga = wrapbuffer_fd(red_fd, DST_W, DST_H, RK_FORMAT_RGB_888, DST_W, DST_H);
        img_src = cv::Mat(DST_H, DST_W, CV_8UC3, ((DMABufferdst *)red_virt)->data);
}

void DMAFrameQueue::get_frame(rga_buffer_t dst_rga[DMA_QUEUE_CAPACITY], rga_buffer_t red_rga)
{
    int tem = -1;
    int k = -1;

    // Check if any frame is ready
    bool found = false;

    std::lock_guard<std::mutex> lock(mtx);
    for(int i=0; i<buf_num; i++)
    {
        if(bufdst[i]->have == 1)
        {
            if(tem==-1)
            {
                tem = bufdst[i]->frame_id;
                k = i;
                found = true;
            }
            if(!found || bufdst[i]->frame_id < tem)
            {
                tem = bufdst[i]->frame_id;
                k = i;
                found = true;
            }
        }
    }
    if (found && k > 0)
    {
        printf("获取到帧 id=%d, 正在复制到 red_rga\n", bufdst[k]->frame_id);
        int ret = imcopy(this->dst_rga[k], red_rga);
        if (ret == IM_STATUS_SUCCESS) 
        {
            bufdst[k]->have = 0;
            bufdst[k]->frame_id=-1;
        } 
        else 
        {
            printf("imcopy running failed, %s\n", imStrError((IM_STATUS)ret));
        }
    }
}

void DMAFrameQueue::free() 
{
    for(int i=0; i<buf_num; i++)
    {
        dma_buf_free(sizeof(DMABuffersrc), &src_fd[i], src_virt[i]);
        dma_buf_free(sizeof(DMABufferdst), &dst_fd[i], dst_virt[i]);
    }
    dma_buf_free(sizeof(DMABufferdst), &red_fd, red_virt);
}


