from ultralytics import YOLO

# 将格式转化为onnx便于C++调用
model = YOLO('../runs/detect/train4/weights/best.pt')
model.export(format='onnx')