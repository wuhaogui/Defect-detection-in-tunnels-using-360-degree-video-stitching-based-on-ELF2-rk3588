#pragma once
#include <opencv2/opencv.hpp>
#include <mutex>

constexpr int queue_capacity = 30;
constexpr int num_cameras = 4;

struct framepair {
    cv::Mat frames[num_cameras];
    int frame_id = -1;
};

struct framequeue {
    framepair framequ[queue_capacity];
    std::mutex mtx;
    int count;

    void frame_queue_init();
    void frame_push(const framepair& pair);
    bool frame_take(framepair& out);
    void frame_free();
};
