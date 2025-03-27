import os
import shutil

root_path = "../ships_dataset"
image_paths = [
    os.path.join(root_path, "test", "labels"),
    os.path.join(root_path, "train", "labels"),
    os.path.join(root_path, "valid", "labels"),
]

all_images = r"D:\Playground\bin\dataset\MVDD13-JPEGImages"

def copy_file(pic_name, target):
    source = os.path.join(all_images, pic_name + ".jpg")
    dest = os.path.join(target, pic_name + ".jpg")
    shutil.copy(source, dest)

def print_progress(func, dir_content, target):
    work_num = len(dir_content)
    for i in range(work_num):
        current_progress = i + 1
        num_hashes = (current_progress * 50) // work_num
        percent = (current_progress * 100) // work_num
        progress_bar = "|" + "#" * num_hashes + " " * (50 - num_hashes) + "|"
        print(f"Progress: {progress_bar} {percent}%", end='\r')
        func(dir_content[i], target)
    print()

def main():
    for image_path in image_paths:
        dir_content = []
        for entry in os.scandir(image_path):
            if entry.is_file():
                dir_content.append(entry.name.split('.')[0])
        
        target = os.path.join(os.path.dirname(os.path.dirname(image_path)), "images")
        print(f"Target is: {target}")
        print_progress(copy_file, dir_content, target)

if __name__ == "__main__":
    main()
