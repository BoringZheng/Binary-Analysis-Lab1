# loader_demo

作业目标：
1. 转储任意段内容（十六进制）  
2. 打印数据符号（本地/全局/弱），并与 `readelf` 交叉验证

## 依赖
- Linux（建议 Debian/Ubuntu）
- CMake ≥ 3.13
- g++ ≥ 9
- binutils-dev（提供 libbfd 与头文件）
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake binutils-dev
