#!/bin/bash

# 归档压缩文件查找和统计脚本
# 用法：./find-archives.sh [目录]

SEARCH_DIR="${1:-.}"

echo "搜索目录：$SEARCH_DIR"
echo ""

# 统计各种压缩格式的数量
echo "=== 压缩文件统计 ==="
echo "ZIP 文件:"
find "$SEARCH_DIR" -type f -name "*.zip" 2>/dev/null | wc -l

echo ""
echo "RAR 文件:"
find "$SEARCH_DIR" -type f -name "*.rar" 2>/dev/null | wc -l

echo ""
echo "7z 文件:"
find "$SEARCH_DIR" -type f -name "*.7z" 2>/dev/null | wc -l

echo ""
echo "GZIP 压缩的 tar 文件:"
find "$SEARCH_DIR" -type f \( -name "*.tar.gz" -o -name "*.tgz" \) 2>/dev/null | wc -l

echo ""
echo "XZ 压缩的 tar 文件:"
find "$SEARCH_DIR" -type f \( -name "*.tar.xz" -o -name "*.txz" \) 2>/dev/null | wc -l

echo ""
echo "BZIP2 压缩的 tar 文件:"
find "$SEARCH_DIR" -type f \( -name "*.tar.bz2" -o -name "*.tbz" -o -name "*.tbz2" \) 2>/dev/null | wc -l

echo ""
echo "=== 最大压缩包 ==="
find "$SEARCH_DIR" -type f \( \
    -name "*.zip" -o -name "*.rar" -o -name "*.7z" -o -name "*.tar.gz" \
    -o -name "*.tar.xz" -o -name "*.tar.bz2" -o -name "*.tar.zst" \
\) -exec du -h {} + 2>/dev/null | sort -rh | head -5
