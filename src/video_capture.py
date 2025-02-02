import numpy as np
from hikvisionapi import Client
import cv2
import posix_ipc
import mmap
import struct
import time
import logging
import argparse
import os
from datetime import datetime
import sys

class VideoCapture:
    def __init__(self, width, height, host, username, password, log_dir):
        self.width = width
        self.height = height
        self.frame_size = width * height * 3  # BGR format
        
        # Create logs directory if it doesn't exist
        os.makedirs(log_dir, exist_ok=True)
        
        # Setup logging
        log_file = os.path.join(log_dir, 'capture.log')
        logging.basicConfig(
            level=logging.INFO,
            format='%(asctime)s [%(levelname)s] %(message)s',
            handlers=[
                logging.FileHandler(log_file),
                logging.StreamHandler()
            ]
        )
        self.logger = logging.getLogger(__name__)
        
        # Log initial configuration
        self.logger.info("=" * 50)
        self.logger.info("Initializing VideoCapture")
        self.logger.info(f"Resolution: {width}x{height}")
        self.logger.info(f"Frame size: {self.frame_size} bytes")
        self.logger.info(f"Host: {host}")
        self.logger.info(f"Log directory: {log_dir}")
        
        try:
            # Add a mutex semaphore for synchronizing frame access
            self.mutex = posix_ipc.Semaphore(
                "/live_alert_mutex",
                posix_ipc.O_CREAT,
                initial_value=1
            )
            # Create shared memory
            self.shm = posix_ipc.SharedMemory(
                "/video_stream", 
                posix_ipc.O_CREAT, 
                size=self.frame_size + 4  # +4 for frame counter
            )
            
            # Map the memory
            self.mapped = mmap.mmap(self.shm.fd, self.shm.size)
            self.shm.close_fd()  # We can close file descriptor after mapping
            self.logger.info("Shared memory initialized successfully")
            
            # Connect to camera
            self.logger.info(f"Connecting to camera at {host}...")
            self.cam = Client('http://' + host, username, password)
            self.logger.info("Camera connection established")
            
        except posix_ipc.Error as e:
            self.logger.error(f"Shared memory error: {str(e)}")
            raise
        except Exception as e:
            self.logger.error(f"Initialization failed: {str(e)}")
            raise
    
    def __del__(self):
        try:
            if hasattr(self, 'mapped'):
                self.mapped.close()
                self.logger.info("Mapped memory closed")
            try:
                posix_ipc.SharedMemory("/video_stream").unlink()
                self.logger.info("Shared memory unlinked")
            except posix_ipc.Error as e:
                self.logger.warning(f"Failed to unlink shared memory: {str(e)}")
            self.logger.info("Cleanup completed")
        except Exception as e:
            self.logger.error(f"Cleanup failed: {str(e)}")
    
    def capture_frames(self, test=False):
        counter = 0
        frames_processed = 0
        last_log_time = time.time()
        fps_log_interval = 60  # Log FPS every minute
        
        self.logger.info("Starting frame capture loop")
        
        while True:
            try:
                # Get frame from camera
                vid = self.cam.Streaming.channels[301].picture(method='get', type='opaque_data')
                if vid is not None:
                    # Convert to numpy array
                    frame = np.asarray(bytearray(vid.content), dtype="uint8")
                    frame = cv2.imdecode(frame, cv2.IMREAD_COLOR)
                    
                    if frame is not None:
                        # Resize frame
                        frame = cv2.resize(frame, (self.width, self.height))
                        
                        # Write frame and counter atomically
                        self.mutex.acquire()
                        try:
                            self.mapped.seek(0)
                            self.mapped.write(frame.tobytes())
                            self.mapped.write(struct.pack('I', counter))
                            counter = (counter + 1) % 0xFFFFFFFF
                            frames_processed += 1
                            
                            # Log FPS periodically
                            current_time = time.time()
                            if current_time - last_log_time >= fps_log_interval:
                                fps = frames_processed / (current_time - last_log_time)
                                self.logger.info(f"Processing at {fps:.2f} FPS")
                                frames_processed = 0
                                last_log_time = current_time
                            
                        except Exception as e:
                            self.logger.error(f"Failed to write frame: {str(e)}")
                            raise
                        finally:
                            self.mutex.release()

                        # Optional: show the image for testing
                        if test:
                            cv2.imshow('Frame', frame)
                            if cv2.waitKey(1) & 0xFF == ord('q'):
                                self.logger.info("Test mode: Quit signal received")
                                break
                    else:
                        self.logger.warning("Failed to decode frame")
                else:
                    self.logger.warning("No frame received from camera")
            
            except Exception as e:
                self.logger.error(f"Frame capture error: {str(e)}")
                # Wait before retrying
                time.sleep(1)
            
            time.sleep(0.65)  # Adjust timing as needed

def parse_arguments():
    parser = argparse.ArgumentParser(description='Video Capture Service')
    parser.add_argument('--width', type=int, default=640,
                        help='Frame width (default: 640)')
    parser.add_argument('--height', type=int, default=480,
                        help='Frame height (default: 480)')
    parser.add_argument('--host', type=str, required=True,
                        help='Camera IP address')
    parser.add_argument('--username', type=str, required=True,
                        help='Camera username')
    parser.add_argument('--password', type=str, required=True,
                        help='Camera password')
    parser.add_argument('--log-dir', type=str, default='./logs',
                        help='Directory for log files')
    parser.add_argument('--test', action='store_true',
                        help='Enable test mode with visual output')
    return parser.parse_args()

if __name__ == "__main__":
    try:
        args = parse_arguments()
        
        capture = VideoCapture(
            width=args.width,
            height=args.height,
            host=args.host,
            username=args.username,
            password=args.password,
            log_dir=args.log_dir
        )
        
        capture.capture_frames(test=args.test)
        
    except KeyboardInterrupt:
        print("\nShutting down gracefully...")
        sys.exit(0)
    except Exception as e:
        print(f"Fatal error: {str(e)}")
        sys.exit(1)