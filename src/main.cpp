#include "video_processor.hpp"
#include <iostream>

int main(int argc, char** argv) {
    std::string log_dir = "./logs";  // Default log directory
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg.substr(0, 10) == "--log-dir=") {
            log_dir = arg.substr(10);
        }
    }

    try {
        VideoProcessor processor(640, 480, log_dir);
        processor.processFrames();
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
    return 0;
}