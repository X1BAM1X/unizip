#!/bin/bash

# 解压到指定目录并清理
# 用法：./extract-and-clean.sh [压缩包] [解压目录]

ARCHIVE="$1"
DEST_DIR="${2:-./extracted}"

if [ -z "$ARCHIVE" ]; then
    echo "用法：./extract-and-clean.sh <压缩包> [解压目录]"
    echo ""
    echo "示例:"
    echo "  ./extract-and-clean.sh archive.zip ./output"
    exit 1
fi

if [ ! -f "$ARCHIVE" ]; then
    echo "错误：文件不存在：$ARCHIVE"
    exit 1
fi

echo "解压：$(basename "$ARCHIVE")"
echo "目标目录：$DEST_DIR"
echo ""

# 解压
if uz -o "$DEST_DIR" "$ARCHIVE" -f; then
    echo "✓ 解压成功"
    
    # 显示解压后的文件列表
    echo ""
    echo "解压文件:"
    ls -lh "$DEST_DIR" 2>/dev/null || echo "  (目录为空或解压失败)"
    
    exit 0
else
    echo "✗ 解压失败"
    exit 1
fi
