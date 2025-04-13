import os
import json

def coco_to_yolo(coco_json_path, output_dir):
    # 确保输出文件夹存在，必须在后续写文件之前创建
    os.makedirs(output_dir, exist_ok=True)
    
    with open(coco_json_path, 'r') as f:
        coco_data = json.load(f)
    # 构建 image_id 到图片信息的映射
    images_info = {img['id']: img for img in coco_data['images']}

    # 构建 image_id 对应的所有标注列表
    annotations_by_image = {}
    for ann in coco_data['annotations']:
        image_id = ann['image_id']
        annotations_by_image.setdefault(image_id, []).append(ann)

    # 遍历每个图片
    for image_id, img_info in images_info.items():
        file_name = img_info['file_name']
        img_width = img_info['width']
        img_height = img_info['height']

        anns = annotations_by_image.get(image_id, [])
        yolo_lines = []
        for ann in anns:
            # 使用 COCO 中的 category_id；如有需要可以调整为从 0 开始
            category_id = ann['category_id']
            # COCO 格式 bbox: [x_min, y_min, box_width, box_height]
            x_min, y_min, box_w, box_h = ann['bbox']
            # 计算中心点坐标
            x_center = x_min + box_w / 2.0
            y_center = y_min + box_h / 2.0
            # 归一化到 [0, 1]
            x_center_norm = x_center / img_width
            y_center_norm = y_center / img_height
            w_norm = box_w / img_width
            h_norm = box_h / img_height

            # 格式化为一行字符串（保留6位小数）
            yolo_line = f"{category_id} {x_center_norm:.6f} {y_center_norm:.6f} {w_norm:.6f} {h_norm:.6f}"
            yolo_lines.append(yolo_line)

        # 生成对应图片的 txt 文件，例如 022430.txt 对应 022430.jpg
        base_name = os.path.splitext(file_name)[0]
        output_file = os.path.join(output_dir, base_name + ".txt")
        with open(output_file, 'w') as f_out:
            f_out.write("\n".join(yolo_lines))
            
if __name__ == "__main__":
    coco_json_path = "instances_train2017.json"
    output_dir = "train"
    coco_to_yolo(coco_json_path, output_dir)
    print("转换完成！YOLO 标注文件存放在目录:", output_dir)
