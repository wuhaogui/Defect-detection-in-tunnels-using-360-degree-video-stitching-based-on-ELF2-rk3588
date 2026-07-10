# Defect-detection-in-tunnels-using-360-degree-video-stitching-based-on-ELF2-rk3588
面向无GPS暗光隧道管廊，研发重载巡检空中机器人，采用边缘视觉大脑+飞行小脑异构架构。视觉大脑基于RK3588重构数据流水线，利用dma_heap零拷贝与RGA加速实现多路视频高效缩放转换，部署YOLOv8量化模型于6TOPS NPU，结合OpenCV拼接，机载完成4路环视实时缝合与裂缝捕捉。飞行小脑基于Pixhawk 6C定制PX4固件，融合光流与ToF测距，实现厘米级悬停；针对3.1kg重载引发的高频共振，ULog驱动调优，配置多阶低通滤波器重构PID，确保视觉采集平稳。配合物理隔离供电与主动补光，系统实现即飞即拼即识闭环，验证了RK3588在具身智能中的端侧算力与工程价值。
