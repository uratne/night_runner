# Night Runner

Night Runner is a security camera system that performs human detection during nighttime hours. It consists of a Python-based video capture service that interfaces with Hikvision cameras and a C++ processing service that performs human detection using OpenCV's HOG (Histogram of Oriented Gradients) detector.

## Architecture

The system uses a producer-consumer architecture with two main components:

1. **Video Capture Service (Producer)**
   - Written in Python
   - Connects to Hikvision IP camera
   - Captures frames and writes them to shared memory
   - Uses POSIX shared memory for inter-process communication
   - Implements frame counter for synchronization

2. **Video Processing Service (Consumer)**
   - Written in C++
   - Reads frames from shared memory
   - Performs human detection using OpenCV HOG
   - Saves detection events with timestamps
   - Uses semaphores for thread-safe memory access

### Inter-Process Communication

The system uses two POSIX IPC mechanisms:

1. **Shared Memory** (`/dev/shm/video_stream`)
   - Used for passing video frames between processes
   - Size: width × height × 3 (BGR) + 4 bytes (frame counter)
   - Managed using POSIX shared memory API
   - View current shared memory segments:
     ```bash
     ls -l /dev/shm/
     ipcs -m  # List all shared memory segments
     ```

2. **Semaphores** (`/dev/shm/sem.live_alert_mutex`)
   - Used for synchronizing access to shared memory
   - Prevents race conditions between producer and consumer
   - View current semaphores:
     ```bash
     ls -l /dev/shm/sem.*
     ipcs -s  # List all semaphores
     ```

## Prerequisites

- CMake (>= 3.10)
- OpenCV (with static libraries)
- Python 3.x
- Required Python packages (see requirements.txt)

## Building

1. Create a build directory:
   ```bash
   mkdir build && cd build
   ```

2. Configure CMake:
   ```bash
   cmake ..
   ```

3. Build the project:
   ```bash
   make
   ```

4. Create deployment package:
   ```bash
   make package
   ```

The deployment package will be created as `security_camera.tar` in the build directory.

## Installation

1. Extract the deployment package:
   ```bash
   tar xf security_camera.tar
   cd deploy
   ```

2. Install Python dependencies:
   ```bash
   pip3 install -r requirements.txt
   ```

## Configuration

Configure the camera settings by setting environment variables:

```bash
export CAMERA_HOST=192.x.x.x  # Camera IP address
export CAMERA_USER=admin      # Camera username
export CAMERA_PASS=admin      # Camera password
```

The night runner script (`night_runner.sh`) can be configured by editing these variables:
- `START_HOUR`: Hour to start surveillance (default: 23 / 11 PM)
- `END_HOUR`: Hour to stop surveillance (default: 6 / 6 AM)

## Running

### Manual Operation

1. Start the system:
   ```bash
   ./night_runner.sh
   ```

2. Stop the system:
   ```bash
   ./stop.sh
   ```

### Automated Operation (Cron)

To run the night runner every 5 minutes:

1. Open crontab:
   ```bash
   crontab -e
   ```

2. Add the following line (adjust path as needed):
   ```
   */5 * * * * /path/to/night_runner.sh
   ```

## Monitoring

### Process Status
```bash
# Check if processes are running
ps aux | grep video_capture.py
ps aux | grep video_processor

# Check logs
tail -f logs/capture.log
tail -f logs/processor.log
```

### IPC Resources
```bash
# List all IPC resources
ipcs

# Clean up all IPC resources (use with caution)
ipcrm -a

# Check shared memory usage
df -h /dev/shm
```

## Troubleshooting

### Common Issues

1. **Shared Memory Issues**
   - Check available shared memory:
     ```bash
     cat /proc/sys/kernel/shmmax  # Maximum shared memory segment size
     cat /proc/sys/kernel/shmmni  # Maximum number of shared memory segments
     ```
   - Clean up stale shared memory:
     ```bash
     rm /dev/shm/video_stream
     rm /dev/shm/sem.live_alert_mutex
     ```

2. **Permission Issues**
   - Ensure proper permissions on scripts:
     ```bash
     chmod 755 night_runner.sh stop.sh
     ```

3. **Process Monitoring**
   - Check process status:
     ```bash
     systemctl status cron  # Check if cron is running
     journalctl -u cron    # Check cron logs
     ```

## Performance Tuning

The system can be tuned by adjusting these parameters:

1. Frame Processing:
   - Adjust `sleep_time` in VideoProcessor class for CPU usage
   - Modify `CONFIDENCE_THRESHOLD` for detection sensitivity

2. Memory Usage:
   - Adjust frame resolution in video_capture.py
   - Modify buffer sizes in shared memory allocation

## Security Considerations

1. Camera Access:
   - Use strong passwords
   - Place camera on isolated network
   - Use environment variables for credentials

2. File Permissions:
   - Ensure logs directory has appropriate permissions
   - Protect credential files

## Additional Notes

- The system uses OpenCV's HOG detector which is optimized for human detection
- Detection images are saved with timestamps for later review
- The system automatically handles day/night transitions
- All operations are logged for monitoring and debugging
- The system is designed to recover from crashes and network issues
