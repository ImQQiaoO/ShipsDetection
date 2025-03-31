from ultralytics import YOLO


def inference_test():
    model = YOLO('../runs/detect/train4/weights/best.pt')
    results = model.predict(source='./target/')
    for result in results:
        print(result.boxes.xyxyn) 
        result.show()


if __name__ == '__main__':
    inference_test()

# 检测框: x=254, y=234, width=878, height=322
# 检测框: x=398, y=357, width=338, height=115