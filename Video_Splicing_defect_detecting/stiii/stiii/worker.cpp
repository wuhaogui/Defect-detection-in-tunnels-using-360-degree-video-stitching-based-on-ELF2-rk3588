// worker.cpp - 4 camera stitch using OpenCV Stitcher
#include "worker.h"
#include <thread>
#include <chrono>

void worker(framequeue* frameQ, stitqueue* stitQ, std::atomic<bool>* exitFlag) {
    cv::Ptr<cv::Stitcher> stitcher = cv::Stitcher::create(cv::Stitcher::PANORAMA);

    while (!exitFlag->load()) {
        framepair pair;
        if (!frameQ->frame_take(pair)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        std::vector<cv::Mat> imgs;
        for (int i = 0; i < num_cameras; i++) {
            if (!pair.frames[i].empty()) {
                imgs.push_back(pair.frames[i].clone());
            }
        }

        if (imgs.size() < 2) continue;

        cv::Mat pano;
        cv::Stitcher::Status status = stitcher->stitch(imgs, pano);

        if (status == cv::Stitcher::OK && !pano.empty()) {
            stitpair result;
            result.frame = pano.clone();
            result.frame_id = pair.frame_id;
            stitQ->frame_push(result);
        }
    }
}
