from ultralytics import YOLO


def inference_test():
    model = YOLO('../runs/detect/train4/weights/best.pt')
    results = model.predict(
        source='./target/',
        iou=0.45,
        # save=True,
    )
    for result in results:
        print(result.boxes.xyxyn) 
        result.show()


if __name__ == '__main__':
    inference_test()
