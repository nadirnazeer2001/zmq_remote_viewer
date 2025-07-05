import cv2
import zmq
import time
import numpy as np

def publish_camera_stream(port=5555, camera_index=0):
    # Initialize ZMQ publisher
    context = zmq.Context()
    publisher = context.socket(zmq.PUB)
    publisher.bind(f"tcp://*:{port}")
    
    # Initialize camera
    cap = cv2.VideoCapture(camera_index)
    if not cap.isOpened():
        print("Error: Could not open camera")
        return
    
    print(f"Publishing camera stream on port {port}...")
    try:
        while True:
            # Read frame from camera
            ret, frame = cap.read()
            if not ret:
                print("Error: Could not read frame")
                break
            
            # Convert frame to JPEG
            _, jpeg_frame = cv2.imencode('.jpg', frame, [cv2.IMWRITE_JPEG_QUALITY, 80])
            
            # Send frame
            publisher.send(jpeg_frame.tobytes())
            
            # Optional: Display locally (comment out if running headless)
            cv2.imshow("Publisher Stream", frame)
            if cv2.waitKey(1) & 0xFF == ord('q'):
                break
                
            time.sleep(0.03)  # ~30 FPS
            
    except KeyboardInterrupt:
        print("Stopping stream...")
    finally:
        cap.release()
        cv2.destroyAllWindows()
        publisher.close()
        context.term()

if __name__ == "__main__":
    # Usage: python publisher.py [port] [camera_index]
    import sys
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 5555
    camera_index = int(sys.argv[2]) if len(sys.argv) > 2 else 0
    
    publish_camera_stream(port, camera_index)
