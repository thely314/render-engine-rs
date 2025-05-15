import numpy as np
from PIL import Image

# 设置图像尺寸
width, height = 1024, 1024

# 生成0~255之间的白噪声整数数据（uint8）
noise = np.random.randint(0, 256, (height, width), dtype=np.uint8)

# 创建图像对象（灰度模式 'L'）
img = Image.fromarray(noise, mode='L')

# 保存图像
img.save("noisy_background.png")

print("图像已保存为 noisy_background.png")
# 此py文件仅用于生成噪声纹理