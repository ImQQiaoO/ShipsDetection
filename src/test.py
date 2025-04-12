from ultralytics import YOLO
import time
import cv2

def inference_test():
    model = YOLO('../runs/detect/train4/weights/best.pt')
    results = model.predict(source='./target/')
    for result in results:
        print(result.boxes.xyxyn)
        result.show()


def inference_test_video():
    # 明确指定设备为CPU
    model = YOLO('./runs/detect/train4/weights/best.pt')
    cap = cv2.VideoCapture('./target_video/ship_video.mp4')
    fps_input = cap.get(cv2.CAP_PROP_FPS)

    start_time = time.time()
    total_frames = 0

    while True:
        ret, frame = cap.read()
        if not ret:
            break

        # 也可以在推理时指定设备为CPU
        # results = model(frame, verbose=False, device='cpu')
        results = model(frame, verbose=False)
        res = results[0]
        frame_with_boxes = res.plot()
        cv2.imshow('Inference Video', frame_with_boxes)

        total_frames += 1
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    end_time = time.time()
    total_time = end_time - start_time

    actual_fps = total_frames / total_time
    print(f"视频处理完毕，共处理了 {total_frames} 帧，总耗时 {total_time:.2f} 秒")
    print(f"输入视频帧率 (FPS): {fps_input:.2f}")
    print(f"实际处理帧率 (FPS): {actual_fps:.2f}")
    cap.release()
    cv2.destroyAllWindows()



if __name__ == '__main__':
    # inference_test()
    inference_test_video()

# 检测框: x=254, y=234, width=878, height=322
# 检测框: x=398, y=357, width=338, height=115
