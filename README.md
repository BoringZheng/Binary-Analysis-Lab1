# Binary Analysis Lab 1: Binary Loader

## 项目简介

本项目是二进制分析实验的第一部分，目标是通过使用 [GNU binutils](https://sourceware.org/binutils/) 提供的 libbfd 库实现一个简易的二进制文件加载器，帮助学习 ELF/PE 等目标文件格式。该加载器能够解析二进制文件，列出各节的信息和符号表，并在需要时转储特定节的内容或打印数据符号，便于与系统工具如 `readelf` 交叉验证:contentReference[oaicite:0]{index=0}。

**主要功能：**

- 列出可执行文件的基本信息（类型、架构、入口地址等）以及各个节的名称、虚拟地址和大小；
- 解析符号表，区分函数符号和对象（数据）符号，并标注符号的绑定（本地/全局/弱）；
- 通过命令行选项 `--dump-section <节名>` 将指定节的内容以十六进制和 ASCII 形式输出；
- 通过命令行选项 `--print-data-symbols` 只打印数据符号，方便分析全局和本地变量。

## 环境依赖

要编译和运行本项目，建议使用类 Unix 系统（如 Debian/Ubuntu）。需要以下依赖:contentReference[oaicite:1]{index=1}：

- `CMake` ≥ 3.13；  
- `g++` ≥ 9；  
- `binutils-dev`：提供 `libbfd` 库及头文件；  
- GNU build 工具链（例如 `build-essential`）。

可以通过以下命令安装依赖：

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake binutils-dev
```

## 构建与安装
1. 克隆本仓库并且进入项目根目录。
2. 使用CMake 生成构建文件并编译：
```bash
mkdir build && cd build
cmake ..
cmake --build . -j
```
执行完成后，会在``./build/``目录下生成可执行文件``loader_demo``。

## 使用方法
基本用法：
```bash
./build/loader_demo <binary_file> [--dump-section] [--print-data-symbols]
```

若不加任何选项，那么将会打印文件的基本信息、节列表以及符号表。
``--dump-section`` 后跟需要转储的节名。
``--print-data-symbols`` ：只输出数据符号，包括变量名、绑定类型和地址，便于分析全局和局部数据。
