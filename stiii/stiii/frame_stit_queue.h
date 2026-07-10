#pragma once
#include <opencv2/opencv.hpp>
#include <mutex>

constexpr int cou = 30;

struct stitpair {
    cv::Mat frame;
    int frame_id = -1;
};

struct stitqueue {
    stitpair framequ[cou];
    std::mutex mtx;
    int count;

    void frame_queue_init();
    void frame_push(const stitpair& pair);
    bool frame_take(stitpair& out);
    void frame_free();
};
