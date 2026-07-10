#include "frame_queue.h"
#include <algorithm>

void framequeue::frame_queue_init() {
    frame_free();
    for (int i = 0; i < queue_capacity; ++i) {
        framequ[i].frame_id = -1;
    }
    count = 0;
}

void framequeue::frame_push(const framepair& pair) {
    std::lock_guard<std::mutex> lock(mtx);
    if (count < queue_capacity) {
        int idx = count++;
        for (int i = 0; i < num_cameras; i++) {
            pair.frames[i].copyTo(framequ[idx].frames[i]);
        }
        framequ[idx].frame_id = pair.frame_id;
    } else {
        int min_idx = 0;
        int min_id = framequ[0].frame_id;
        for (int i = 1; i < queue_capacity; ++i) {
            if (framequ[i].frame_id < min_id) {
                min_id = framequ[i].frame_id;
                min_idx = i;
            }
        }
        for (int i = 0; i < num_cameras; i++) {
            pair.frames[i].copyTo(framequ[min_idx].frames[i]);
        }
        framequ[min_idx].frame_id = pair.frame_id;
    }
    if (count > 0) {
        std::sort(framequ, framequ + count, [](const framepair& a, const framepair& b) {
            return a.frame_id < b.frame_id;
        });
    }
}

bool framequeue::frame_take(framepair& out) {
    std::lock_guard<std::mutex> lock(mtx);
    if (count == 0) return false;

    for (int i = 0; i < num_cameras; i++) {
        framequ[0].frames[i].copyTo(out.frames[i]);
    }
    out.frame_id = framequ[0].frame_id;

    int last = count - 1;
    if (last != 0) {
        framequ[0] = std::move(framequ[last]);
    }
    for (int i = 0; i < num_cameras; i++) {
        framequ[last].frames[i].release();
    }
    framequ[last].frame_id = -1;
    --count;

    if (count > 0) {
        std::sort(framequ, framequ + count, [](const framepair& a, const framepair& b) {
            return a.frame_id < b.frame_id;
        });
    }
    return true;
}

void framequeue::frame_free() {
    std::lock_guard<std::mutex> lock(mtx);
    for (int i = 0; i < queue_capacity; ++i) {
        for (int j = 0; j < num_cameras; j++) {
            framequ[i].frames[j].release();
        }
        framequ[i].frame_id = -1;
    }
    count = 0;
}
