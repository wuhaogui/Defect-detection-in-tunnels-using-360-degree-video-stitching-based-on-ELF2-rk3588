cmake_minimum_required(VERSION 3.10)
project(projectwithoutyolo LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(THIRD_PARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty)

find_package(OpenCV REQUIRED)
find_package(Threads REQUIRED)

find_path(RGA_INCLUDE_DIR
    NAMES RgaApi.h
    PATHS
    ${THIRD_PARTY_DIR}/librga/include
        /usr/include
        /usr/local/include
)

find_library(RGA_LIB
    NAMES rga
    PATHS
    ${THIRD_PARTY_DIR}/librga/Linux/aarch64
        /usr/lib
        /usr/lib/aarch64-linux-gnu
        /usr/local/lib
)

find_library(RKNNRT_LIB
    NAMES rknnrt
    PATHS
    ${THIRD_PARTY_DIR}/rknpu2/Linux/aarch64
        /usr/lib
        /usr/lib/aarch64-linux-gnu
        /usr/local/lib
)

find_library(TURBOJPEG_LIB
    NAMES turbojpeg
    PATHS
        ${THIRD_PARTY_DIR}/jpeg_turbo/Linux/aarch64
        /usr/lib
        /usr/lib/aarch64-linux-gnu
        /usr/local/lib
)

if(NOT RGA_INCLUDE_DIR)
    message(FATAL_ERROR "Cannot find RGA headers (RgaApi.h).")
endif()

if(NOT RGA_LIB)
    message(FATAL_ERROR "Cannot find librga.")
endif()

if(NOT RKNNRT_LIB)
    message(FATAL_ERROR "Cannot find librknnrt.")
endif()

if(NOT TURBOJPEG_LIB)
    message(FATAL_ERROR "Cannot find libturbojpeg.")
endif()

add_executable(app
    thread.cpp
    include_create/worker.cpp
    include_create/dma_queue.cpp
    include_yolo/dma_alloc.cpp
    include_create/yolov8-pose.cc
    until/preprocess.cc
    include_yolo/postprocess.cc
    until/file_utils.c
    until/image_utils.c
    until/image_drawing.c
)

target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include_create
    ${CMAKE_CURRENT_SOURCE_DIR}/include_yolo
    ${CMAKE_CURRENT_SOURCE_DIR}/until
    ${CMAKE_CURRENT_SOURCE_DIR}/model
    ${OpenCV_INCLUDE_DIRS}
    ${RGA_INCLUDE_DIR}
    ${THIRD_PARTY_DIR}/librga/include
    ${THIRD_PARTY_DIR}/rknpu2/include
    ${THIRD_PARTY_DIR}/stb_image
    ${THIRD_PARTY_DIR}/jpeg_turbo/include
)

target_link_libraries(app PRIVATE
    ${OpenCV_LIBS}
    Threads::Threads
    ${RGA_LIB}
    ${RKNNRT_LIB}
    ${TURBOJPEG_LIB}
    dl
    m
)
