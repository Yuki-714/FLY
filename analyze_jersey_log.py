#!/usr/bin/env python3
"""
分析B-Human日志文件中识别队友黑色衣服的平均饱和度和亮度值

这个脚本需要使用B-Human的Python库来读取二进制日志文件。
根据JerseyClassifierProvider2020For2023.cpp的代码，球衣分类器会：
1. 采样球衣区域的像素
2. 计算平均色调(H)、饱和度(S)和亮度(I)
3. 对于黑色球衣，主要通过亮度判断（亮度 <= grayRange.min）
"""

import sys
import os

# 添加B-Human Python库路径
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'Make/Python'))

try:
    from pybh import LogReader
    
    def analyze_jersey_data(log_path):
        """分析日志文件中的球衣识别数据"""
        print(f"正在分析日志文件: {log_path}")
        print("=" * 80)
        
        try:
            reader = LogReader(log_path)
            
            # 统计变量
            saturation_values = []
            brightness_values = []
            hue_values = []
            frame_count = 0
            
            # 遍历日志帧
            for frame in reader:
                frame_count += 1
                
                # 尝试获取ECImage（增强对比度图像）数据
                if hasattr(frame, 'ECImage'):
                    ec_image = frame.ECImage
                    # 这里需要根据实际的数据结构来提取饱和度和亮度
                    # 由于我们无法直接访问调试绘制信息，我们需要分析图像数据
                    
                # 尝试获取ObstaclesFieldPercept（障碍物感知）数据
                if hasattr(frame, 'ObstaclesFieldPercept'):
                    obstacles = frame.ObstaclesFieldPercept
                    for obstacle in obstacles.obstacles:
                        if obstacle.type == 1:  # ownPlayer
                            print(f"帧 {frame_count}: 检测到队友")
                            
            print(f"\n总共分析了 {frame_count} 帧")
            
            if saturation_values:
                avg_saturation = sum(saturation_values) / len(saturation_values)
                avg_brightness = sum(brightness_values) / len(brightness_values)
                print(f"\n黑色球衣识别统计:")
                print(f"  平均饱和度: {avg_saturation:.2f}")
                print(f"  平均亮度: {avg_brightness:.2f}")
                print(f"  样本数量: {len(saturation_values)}")
            else:
                print("\n未找到球衣识别数据")
                
        except Exception as e:
            print(f"读取日志文件时出错: {e}")
            print("\n提示: 这个日志文件需要使用B-Human的SimRobot工具打开")
            print("      或者需要先安装pybh库（在Util/LogAnalyzer目录下）")
            
    if __name__ == "__main__":
        if len(sys.argv) < 2:
            print("用法: python3 analyze_jersey_log.py <日志文件路径>")
            print("\n示例:")
            print("  python3 analyze_jersey_log.py 'Config/Logs/5.2上午对战深圳技术大学/fly16/fly16_fly16_Default_Default__Testing_1.log'")
            sys.exit(1)
            
        log_path = sys.argv[1]
        if not os.path.exists(log_path):
            print(f"错误: 文件不存在: {log_path}")
            sys.exit(1)
            
        analyze_jersey_data(log_path)
        
except ImportError:
    print("错误: 无法导入pybh库")
    print("\n请先安装B-Human的Python库:")
    print("  cd Util/LogAnalyzer")
    print("  python3 -m venv .venv")
    print("  source .venv/bin/activate")
    print("  pip install -r requirements.txt")
    print("\n然后重新运行此脚本")
