#!/bin/bash

# Configuration
START_HOUR=23  # 11 PM
END_HOUR=6     # 6 AM
INSTALL_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"  # Get script directory
PID_FILE="/tmp/night_camera.pid"

# Function to check if we should be running
should_run() {
    current_hour=$(date +%H)
    current_hour=$((10#$current_hour))  # Force base-10 interpretation
    
    if [ $START_HOUR -gt $END_HOUR ]; then
        # Night spans midnight
        if [ $current_hour -ge $START_HOUR ] || [ $current_hour -lt $END_HOUR ]; then
            return 0  # True
        fi
    else
        # Regular time span
        if [ $current_hour -ge $START_HOUR ] && [ $current_hour -lt $END_HOUR ]; then
            return 0  # True
        fi
    fi
    return 1  # False
}

# Function to check if a specific process is running and is the correct type
is_process_running() {
    local pid=$1
    local expected_name=$2
    
    # First check if process exists
    if [ ! -e "/proc/$pid" ]; then
        return 1
    fi
    
    # Then check if it's our expected process by checking cmdline
    if grep -q "$expected_name" "/proc/$pid/cmdline" 2>/dev/null; then
        return 0
    fi
    
    return 1
}

# Function to check if both processes are running correctly
check_processes() {
    if [ ! -f $PID_FILE ]; then
        echo "PID file not found" >> ./logs/cron.log
        return 1
    fi
    
    read PY_PID CPP_PID < $PID_FILE
    
    # Check Python capture process
    if ! is_process_running $PY_PID "video_capture.py"; then
        echo "Python capture process not running correctly" >> ./logs/cron.log
        return 1
    fi
    
    # Check C++ processor process
    if ! is_process_running $CPP_PID "video_processor"; then
        echo "C++ processor not running correctly" >> ./logs/cron.log
        return 1
    fi
    
    return 0
}

# Function to start the surveillance system
start_surveillance() {
    cd $INSTALL_DIR
    
    # Create logs directory if it doesn't exist
    mkdir -p ./logs
    
    echo "Starting surveillance system at $(date)" >> ./logs/cron.log
    
    # Start Python capture script
    python3 ./video_capture.py \
        --host=${CAMERA_HOST:-192.168.1.64} \
        --username=${CAMERA_USER:-admin} \
        --password=${CAMERA_PASS:-admin123} \
        --log-dir=./logs &
    PY_PID=$!

    # Wait for shared memory to be created
    sleep 3
    
    # Start C++ processor
    ./video_processor --log-dir=./logs &
    CPP_PID=$!
    
    # Brief pause to let processes start
    sleep 1
    
    # Verify both processes started successfully
    if ! is_process_running $PY_PID "video_capture.py" || ! is_process_running $CPP_PID "video_processor"; then
        echo "Failed to start processes at $(date)" >> ./logs/cron.log
        kill $PY_PID 2>/dev/null
        kill $CPP_PID 2>/dev/null
        return 1
    fi
    
    # Save PIDs
    echo "$PY_PID $CPP_PID" > $PID_FILE
    
    echo "Successfully started surveillance system at $(date)" >> ./logs/cron.log
    return 0
}

# Function to stop the surveillance system
stop_surveillance() {
    if [ -f $PID_FILE ]; then
        read PY_PID CPP_PID < $PID_FILE
        # Kill processes if they exist
        if [ -e "/proc/$PY_PID" ]; then
            kill $PY_PID 2>/dev/null
        fi
        if [ -e "/proc/$CPP_PID" ]; then
            kill $CPP_PID 2>/dev/null
        fi
        rm -f $PID_FILE
        echo "Stopped surveillance system at $(date)" >> ./logs/cron.log
    fi
}

# Main logic
SHOULD_BE_RUNNING=$(should_run && echo "yes" || echo "no")
CURRENTLY_RUNNING=$(check_processes && echo "yes" || echo "no")

if [ "$SHOULD_BE_RUNNING" = "yes" ]; then
    if [ "$CURRENTLY_RUNNING" = "no" ]; then
        stop_surveillance  # Cleanup any leftover processes
        start_surveillance
    fi
else
    if [ "$CURRENTLY_RUNNING" = "yes" ]; then
        stop_surveillance
    fi
fi