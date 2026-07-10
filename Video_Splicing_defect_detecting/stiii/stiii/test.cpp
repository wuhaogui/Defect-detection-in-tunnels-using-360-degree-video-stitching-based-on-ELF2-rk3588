// test.cpp - 4 camera real-time stitching
#include <opencv2/opencv.hpp>
#include <iostream>
#include <thread>
#include <atomic>
#include "frame_queue.h"
#include "frame_stit_queue.h"
#include "worker.h"

#define NUM_THREADS 2
using namespace std;
using namespace cv;

atomic<bool> g_exit(false);

int main() {
    VideoCapture cap[num_cameras] = {
        VideoCapture(1),
        VideoCapture(2),
        VideoCapture(3),
        VideoCapture(4)
    };

    for (int i = 0; i < num_cameras; i++) {
        if (!cap[i].isOpened()) {
            cerr << "Camera " << i << " open failed!" << endl;
            return -1;
        }
    }

    framequeue frameQ;
    frameQ.frame_queue_init();
    stitqueue stitQ;
    stitQ.frame_queue_init();

    vector<thread> threads;
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back(worker, &frameQ, &stitQ, &g_exit);
    }

    for (int i = 0; i < num_cameras; i++) {
        namedWindow("Camera" + to_string(i), WINDOW_NORMAL);
    }
    namedWindow("Panorama", WINDOW_NORMAL);

    framepair pair;
    stitpair result;

    while (true) {
        for (int i = 0; i < num_cameras; i++) {
            cap[i] >> pair.frames[i];
        }
        pair.frame_id++;

        for (int i = 0; i < num_cameras; i++) {
            if (!pair.frames[i].empty()) {
                imshow("Camera" + to_string(i), pair.frames[i]);
            }
        }

        frameQ.frame_push(pair);

        if (stitQ.frame_take(result)) {
            if (!result.frame.empty()) {
                imshow("Panorama", result.frame);
            }
        }

        if (waitKey(1) == 27) break;
    }

    g_exit = true;
    for (auto& t : threads) t.join();
    frameQ.frame_free();
    stitQ.frame_free();
    destroyAllWindows();
    return 0;
}
