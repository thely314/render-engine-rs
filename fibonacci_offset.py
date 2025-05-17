import math

# 生成 Fibonacci 螺旋偏移
def generate_fibonacci_disk_samples(count):
    offsets = []
    golden_angle = math.pi * (3 - math.sqrt(5))  # ≈ 2.39996

    for i in range(count):
        r = 1
        theta = i * golden_angle
        x = r * math.cos(theta)
        y = r * math.sin(theta)
        offsets.append((x, y))

    return offsets

# 保存为 txt 文件
def save_to_txt(filename, data):
    with open(filename, 'w') as f:
        for x, y in data:
            f.write("{")
            f.write(f"{x:.6f}f, {y:.6f}f")
            f.write("},")

# 主函数
if __name__ == "__main__":
    sample_count = 64
    offsets = generate_fibonacci_disk_samples(sample_count)
    save_to_txt("fibonacci_offsets.txt", offsets)
    print(f"Saved {sample_count} offsets to fibonacci_offsets.txt")

# 此py文件仅用于生成斐波那契偏移数列作为constexpr变量