#!/bin/bash

# 批量解压脚本
# 用法：./batch-extract.sh [压缩包目录] [目标目录]

ARCHIVE_DIR="${1:-.}"
DEST_DIR="${2:-./extracted}"

echo "批量解压脚本"
echo "源目录：$ARCHIVE_DIR"
echo "目标目录：$DEST_DIR"
echo ""

# 创建目标目录
mkdir -p "$DEST_DIR"

# 处理 zip 文件
echo "=== 处理 zip 文件 ==="
for file in "$ARCHIVE_DIR"/*.zip; do
    [ -f "$file" ] || continue
    basename=$(basename "$file" .zip)
    uz "$file" "$DEST_DIR/$basename" -f 2>/dev/null && echo "✓ $file" || echo "✗ $file"
done

# 处理 tar.gz 文件
echo ""
echo "=== 处理 tar.gz 文件 ==="
for file in "$ARCHIVE_DIR"/*.tar.gz "$ARCHIVE_DIR"/*.tgz; do
    [ -f "$file" ] || continue
    basename=$(basename "$file")
    uz -o "$DEST_DIR" "$file" -f 2>/dev/null && echo "✓ $file" || echo "✗ $file"
done

# 处理 tar.xz 文件
echo ""
echo "=== 处理 tar.xz 文件 ==="
for file in "$ARCHIVE_DIR"/*.tar.xz "$ARCHIVE_DIR"/*.txz; do
    [ -f "$file" ] || continue
    basename=$(basename "$file")
    uz -o "$DEST_DIR" "$file" -f 2>/dev/null && echo "✓ $file" || echo "✗ $file"
done

# 处理 7z 文件
echo ""
echo "=== 处理 7z 文件 ==="
for file in "$ARCHIVE_DIR"/*.7z; do
    [ -f "$file" ] || continue
    basename=$(basename "$file" .7z)
    uz -o "$DEST_DIR" "$file" -f 2>/dev/null && echo "✓ $file" || echo "✗ $file"
done

# 处理 rar 文件
echo ""
echo "=== 处理 rar 文件 ==="
for file in "$ARCHIVE_DIR"/*.rar; do
    [ -f "$file" ] || continue
    basename=$(basename "$file" .rar)
    uz -o "$DEST_DIR" "$file" -f 2>/dev/null && echo "✓ $file" || echo "✗ $file"
done

echo ""
echo "=== 处理完成 ==="
echo "解压文件位于：$DEST_DIR"
