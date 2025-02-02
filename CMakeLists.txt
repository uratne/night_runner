cmake_minimum_required(VERSION 3.10)
project(SecurityCamera)

# Set C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Static linking configuration
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
set(OpenCV_STATIC ON)
set(BUILD_SHARED_LIBS OFF)

# Find OpenCV package
find_package(OpenCV REQUIRED)

# Include directories
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/main.cpp
    src/logger.cpp
    src/video_processor.cpp
)

# Add C++ executable
add_executable(video_processor ${SOURCES})

# Link libraries for C++ executable
target_link_libraries(video_processor 
    ${OpenCV_LIBS}
    rt  # For shared memory
    pthread 
    dl
    z
)

# Set deployment directory
set(DEPLOY_DIR "${CMAKE_BINARY_DIR}/deploy")

# Create deployment directories
add_custom_command(
    TARGET video_processor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${DEPLOY_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${DEPLOY_DIR}/logs
)

# Copy files to deploy directory
add_custom_command(
    TARGET video_processor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy 
        $<TARGET_FILE:video_processor>
        ${CMAKE_SOURCE_DIR}/src/video_capture.py
        ${CMAKE_SOURCE_DIR}/scripts/night_runner.sh
        ${CMAKE_SOURCE_DIR}/scripts/stop.sh
        ${DEPLOY_DIR}/
)

# Set executable permissions for scripts
add_custom_command(
    TARGET video_processor POST_BUILD
    COMMAND chmod 755 ${DEPLOY_DIR}/night_runner.sh
    COMMAND chmod 755 ${DEPLOY_DIR}/stop.sh
)

# Create requirements.txt
file(WRITE "${DEPLOY_DIR}/requirements.txt"
"numpy
opencv-python
hikvisionapi
posix_ipc")

# Create deployment package
add_custom_target(package
    COMMAND ${CMAKE_COMMAND} -E tar cf security_camera.tar --format=gnutar deploy
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Creating deployment package"
    DEPENDS video_processor
)

# Print status information
message(STATUS "OpenCV library status:")
message(STATUS "    version: ${OpenCV_VERSION}")
message(STATUS "    libraries: ${OpenCV_LIBS}")
message(STATUS "    include path: ${OpenCV_INCLUDE_DIRS}")