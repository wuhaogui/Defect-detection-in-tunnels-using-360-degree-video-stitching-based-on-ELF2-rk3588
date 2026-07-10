#pragma once
#include <opencv2/opencv.hpp>
#include <atomic>
#include "frame_queue.h"
#include "frame_stit_queue.h"

void worker(framequeue* frameQ, stitqueue* stitQ, std::atomic<bool>* exitFlag);
