import matplotlib.pyplot as plt

plt.rcParams['font.sans-serif'] = ['SimHei']  # 设置字体为黑体，解决中文乱码问题
plt.rcParams['axes.unicode_minus'] = False  # 显示负号

# 示例数据
x = ['A', 'B', 'C', 'D']
y = [44.08, 21.83, 34.49, 18.93]

bars = plt.bar(x, y, color='skyblue', width=0.35, edgecolor='black')
for bar in bars:
    # 获取柱子高度和x位置
    height = bar.get_height()
    x_pos = bar.get_x() + bar.get_width() / 2
    plt.text(x_pos, height, str(height), ha='center', va='bottom')

# 添加标题和标签
plt.title('不同方式调用模型推理的帧数表现')
plt.xlabel('类别')
plt.ylabel('FPS')

# 显示图形
plt.show()
