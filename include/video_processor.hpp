#pragma once
#include <opencv2/opencv.hpp>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <semaphore.h>
#include "logger.hpp"

class VideoProcessor {
private:
    int shm_fd;
    void* mapped_memory;
    size_t frame_size;
    cv::Mat frame;
    int width;
    int height;
    cv::HOGDescriptor hog;
    const float CONFIDENCE_THRESHOLD = 0.2;
    std::string save_directory;
    useconds_t sleep_time = 500;
    Logger logger;
    sem_t* mutex;

    std::string getCurrentTimestamp();
    bool detectHumans(const cv::Mat& frame, std::vector<cv::Rect>& detected);
    void saveFrame(const cv::Mat& frame, const std::vector<cv::Rect>& humans);

public:
    VideoProcessor(int w, int h, const std::string& log_dir);
    void processFrames();
    ~VideoProcessor();
};