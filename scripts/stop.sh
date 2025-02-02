#!/bin/bash

# Stop the producer (Python process) if running
if [ -f producer.pid ]; then
    pid=$(cat producer.pid)
    if ps -p $pid > /dev/null; then
        echo "Stopping producer process (PID: $pid)..."
        kill $pid
        rm producer.pid
    else
        echo "Producer process not running, cleaning up PID file..."
        rm producer.pid
    fi
fi

# Stop the consumer (C++ process) if running
if [ -f consumer.pid ]; then
    pid=$(cat consumer.pid)
    if ps -p $pid > /dev/null; then
        echo "Stopping consumer process (PID: $pid)..."
        kill $pid
        rm consumer.pid
    else
        echo "Consumer process not running, cleaning up PID file..."
        rm consumer.pid
    fi
fi

# Give processes time to cleanup and exit gracefully
sleep 1

# Clean up POSIX shared memory
if [ -e /dev/shm/video_stream ]; then
    rm /dev/shm/video_stream
    echo "Removed shared memory: video_stream"
fi

# Clean up POSIX semaphore
if [ -e /dev/shm/sem.frame_mutex ]; then
    rm /dev/shm/sem.frame_mutex
    echo "Removed semaphore: frame_mutex"
fi

echo "Cleanup completed"