#include "frame_stit_queue.h"
#include <algorithm>

void stitqueue::frame_queue_init() {
    frame_free();
    for (int i = 0; i < cou; ++i) {
        framequ[i].frame_id = -1;
    }
    count = 0;
}

void stitqueue::frame_push(const stitpair& pair) {
    std::lock_guard<std::mutex> lock(mtx);
    if (count < cou) {
        int idx = count++;
        pair.frame.copyTo(framequ[idx].frame);
        framequ[idx].frame_id = pair.frame_id;
    } else {
        int min_idx = 0;
        int min_id = framequ[0].frame_id;
        for (int i = 1; i < cou; ++i) {
            if (framequ[i].frame_id < min_id) {
                min_id = framequ[i].frame_id;
                min_idx = i;
            }
        }
        pair.frame.copyTo(framequ[min_idx].frame);
        framequ[min_idx].frame_id = pair.frame_id;
    }
    if (count > 0) {
        std::sort(framequ, framequ + count, [](const stitpair& a, const stitpair& b) {
            return a.frame_id < b.frame_id;
        });
    }
}

bool stitqueue::frame_take(stitpair& out) {
    std::lock_guard<std::mutex> lock(mtx);
    if (count == 0) return false;

    framequ[0].frame.copyTo(out.frame);
    out.frame_id = framequ[0].frame_id;

    int last = count - 1;
    if (last != 0) {
        framequ[0] = std::move(framequ[last]);
    }
    framequ[last].frame.release();
    framequ[last].frame_id = -1;
    --count;

    if (count > 0) {
        std::sort(framequ, framequ + count, [](const stitpair& a, const stitpair& b) {
            return a.frame_id < b.frame_id;
        });
    }
    return true;
}

void stitqueue::frame_free() {
    std::lock_guard<std::mutex> lock(mtx);
    for (int i = 0; i < cou; ++i) {
        framequ[i].frame.release();
        framequ[i].frame_id = -1;
    }
    count = 0;
}
