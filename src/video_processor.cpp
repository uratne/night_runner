#include "video_processor.hpp"
#include <cstring>
#include <iostream>

VideoProcessor::VideoProcessor(int w, int h, const std::string& log_dir) : 
    width(w),
    height(h),
    frame_size(w * h * 3),
    frame(h, w, CV_8UC3),
    logger(log_dir)
{
    save_directory = log_dir + "/detections";
    mkdir(save_directory.c_str(), 0777);

    mutex = sem_open("/live_alert_mutex", 0);
    if (mutex == SEM_FAILED) {
        logger.log(Logger::ERROR, "Failed to open semaphore: " + std::string(strerror(errno)));
        throw std::runtime_error("Could not open semaphore");
    }

    logger.log(Logger::INFO, "Initializing VideoProcessor");
    logger.log(Logger::INFO, "Save directory: " + save_directory);

    hog.setSVMDetector(cv::HOGDescriptor::getDefaultPeopleDetector());
    
    shm_fd = shm_open("/video_stream", O_RDONLY, 0666);
    if (shm_fd == -1) {
        logger.log(Logger::ERROR, "Failed to open shared memory: " + std::string(strerror(errno)));
        throw std::runtime_error("Could not open shared memory");
    }

    mapped_memory = mmap(NULL, frame_size + 4, PROT_READ, MAP_SHARED, shm_fd, 0);
    if (mapped_memory == MAP_FAILED) {
        close(shm_fd);
        logger.log(Logger::ERROR, "Failed to map memory: " + std::string(strerror(errno)));
        throw std::runtime_error("Could not map memory");
    }
    
    logger.log(Logger::INFO, "VideoProcessor initialized successfully");
}

std::string VideoProcessor::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y%m%d_%H%M%S");
    return ss.str();
}

void VideoProcessor::saveFrame(const cv::Mat& frame, const std::vector<cv::Rect>& humans) {
    std::string timestamp = getCurrentTimestamp();
    std::string filename = save_directory + "/detected_" + timestamp + ".jpg";
    
    cv::Mat save_frame = frame.clone();
    for (const auto& human : humans) {
        cv::rectangle(save_frame, human.tl(), human.br(), cv::Scalar(0, 255, 0), 2);
    }
    
    if (cv::imwrite(filename, save_frame)) {
        logger.log(Logger::INFO, "Saved detection to: " + filename);
    } else {
        logger.log(Logger::ERROR, "Failed to save detection to: " + filename);
    }
}

bool VideoProcessor::detectHumans(const cv::Mat& frame, std::vector<cv::Rect>& detected) {
    std::vector<cv::Rect> found;
    std::vector<double> weights;

    hog.detectMultiScale(frame, found, weights, 0.3,
                        cv::Size(8,8), cv::Size(32,32), 1.05, 2.5, false);

    for (size_t i = 0; i < found.size(); i++) {
        if (weights[i] >= CONFIDENCE_THRESHOLD) {
            cv::Rect r = found[i];
            r.x += r.width/20;
            r.width = r.width*9/10;
            r.y += r.height/20;
            r.height = r.height*9/10;
            detected.push_back(r);
        }
    }

    return !detected.empty();
}

void VideoProcessor::processFrames() {
    uint32_t last_counter = 0;
    int consecutive_detections = 0;
    const int REQUIRED_CONSECUTIVE = 3;
    
    logger.log(Logger::INFO, "Starting frame processing loop");
    
    while (true) {
        if (sem_wait(mutex) == -1) {
            logger.log(Logger::ERROR, "Failed to wait on semaphore: " + std::string(strerror(errno)));
            break;
        }

        try {
            uint32_t current_counter;
            memcpy(&current_counter, 
                static_cast<char*>(mapped_memory) + frame_size, 
                sizeof(uint32_t));
            
            if (current_counter != last_counter) {
                memcpy(frame.data, mapped_memory, frame_size);
                
                cv::Mat blurred;
                cv::GaussianBlur(frame, blurred, cv::Size(3,3), 0);
                
                std::vector<cv::Rect> humans;
                if (detectHumans(blurred, humans)) {
                    consecutive_detections++;
                    if (consecutive_detections >= REQUIRED_CONSECUTIVE) {
                        std::stringstream ss;
                        ss << "Humans detected: " << humans.size();
                        logger.log(Logger::INFO, ss.str());
                        saveFrame(frame, humans);
                    }
                } else {
                    consecutive_detections = 0;
                }
                
                last_counter = current_counter;
            }
            
            sem_post(mutex);
            usleep(sleep_time);
        }
        catch(const std::exception& e) {
            sem_post(mutex);
            std::cerr << e.what() << '\n';
        }
    }
}

VideoProcessor::~VideoProcessor() {
    if (mapped_memory != MAP_FAILED) {
        munmap(mapped_memory, frame_size + 4);
    }
    if (shm_fd != -1) {
        close(shm_fd);
    }
    logger.log(Logger::INFO, "VideoProcessor shut down");
}