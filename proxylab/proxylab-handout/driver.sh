#!/bin/bash

# 设置一些变量
PROXY=./proxy         # 你的 proxy 可执行文件
TINY=./tiny           # 提供测试内容的 tiny web server
DRIVER=./driver.py    # 驱动测试脚本，CS:APP 官方提供
TRACES=./traces       # 请求的 HTTP trace 路径
OUTPUT_DIR=./output   # 保存 proxy 输出的目录

# 检查 proxy 和 tiny 是否存在
if [ ! -x "$PROXY" ]; then
    echo "Error: proxy not found or not executable"
    exit 1
fi

if [ ! -x "$TINY" ]; then
    echo "Error: tiny not found or not executable"
    exit 1
fi

# 杀掉已经运行的 proxy 或 tiny
pkill -f proxy
pkill -f tiny

# 清理旧输出
rm -rf $OUTPUT_DIR
mkdir -p $OUTPUT_DIR

# 启动 tiny server（后台运行）
echo "Starting tiny server..."
$TINY &> $OUTPUT_DIR/tiny.log &
TINYPID=$!
sleep 1

# 启动你的 proxy（后台运行）
echo "Starting proxy..."
$PROXY &> $OUTPUT_DIR/proxy.log &
PROXYPID=$!
sleep 1

# 启动测试脚本
echo "Running driver..."
python3 $DRIVER -p $PROXY -t $TINY -d $OUTPUT_DIR

# 清理
echo "Killing proxy and tiny..."
kill $TINYPID
kill $PROXYPID

echo "Done. Check $OUTPUT_DIR for logs."
