/**
 * yolo_wrapper.h - YOLO推理简单封装
 *
 * 提供给Python ctypes调用的简单接口
 */

#ifndef YOLO_WRAPPER_H
#define YOLO_WRAPPER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 初始化YOLO模型
// model_path: .rknn模型文件路径
// 返回: 0成功, 负值失败
int init_yolo_model(const char* model_path);

// 执行推理
// image_data: RGB图像数据 (HxWx3)
// width, height: 图像尺寸
// results: 输出结果数组
// 返回: 0成功, 负值失败
int yolo_inference(uint8_t* image_data, int width, int height, void* results);

// 释放模型
void release_yolo_model(void);

// 获取结果数量
int get_result_count(void);

#ifdef __cplusplus
}
#endif

#endif // YOLO_WRAPPER_H