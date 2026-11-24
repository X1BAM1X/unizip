#!/bin/bash

# 压缩文件校验脚本
# 用法：./check-integrity.sh [压缩包目录]

CHECK_DIR="${1:-.}"

echo "压缩文件完整性校验"
echo "检查目录：$CHECK_DIR"
echo ""

success=0
failed=0

for file in "$CHECK_DIR"/*.zip "$CHECK_DIR"/*.rar "$CHECK_DIR"/*.7z \
            "$CHECK_DIR"/*.tar.gz "$CHECK_DIR"/*.tar.xz "$CHECK_DIR"/*.tar.bz2 \
            "$CHECK_DIR"/*.tgz "$CHECK_DIR"/*.txz "$CHECK_DIR"/*.tbz* "$CHECK_DIR"/*.gz; do
    [ -f "$file" ] || continue
    
    echo -n "检查 $(basename "$file"): "
    if uz -t "$file" >/dev/null 2>&1; then
        echo "✓ OK"
        ((success++))
    else
        echo "✗ FAILED"
        ((failed++))
    fi
done

echo ""
echo "=== 校验结果 ==="
echo "通过：$success"
echo "失败：$failed"
echo "总计：$((success + failed))"

if [ $failed -gt 0 ]; then
    echo ""
    echo "警告：存在损坏的压缩文件！"
fi
