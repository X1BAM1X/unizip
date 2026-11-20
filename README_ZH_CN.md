# unizip (uz)

[English](README.md) | 简体中文

## 为什么要做这个项目

你是否遇到过这种情况：

```bash
# zip 文件要用 unzip
unzip archive.zip

# rar 文件要用 unrar
unrar l archive.rar

# 7z 文件要用 7z
7z l archive.7z

# tar.gz 文件要用 tar
tar -tzf archive.tar.gz

# 还有 gzip、bzip2、xz、zstd 等...
```

**每个压缩格式都有自己专属的命令，语法完全不同**：

- `unzip` 支持 `-o` `-f` `-l` 等选项
- `unrar` 的语法又不一样
- `7z` 又是一套参数
- `tar` 系列更是花样繁多

**切换命令的痛苦**：每次解压都得先识别格式，然后记对应的命令和参数，效率低下且容易出错。

## 项目优势

`uz` 就是为了解决这个问题而生的**统一解压命令**：

**✓ 统一语法** - 所有格式用同一套命令和参数
```bash
uz archive.zip      # 解压 zip
uz archive.rar      # 解压 rar
uz archive.7z       # 解压 7z
uz archive.tar.gz   # 解压 tar.gz
```

**✓ 自动识别** - 通过文件头魔数自动判断格式，不依赖扩展名

**✓ 无库依赖** - 纯 C 实现，不链接任何解压库，实际调用系统已有命令


**✓ 密码解密** - 支持 `-p` 参数传入解压密码


**✓ 格式全面** - 支持 zip/rar/7z/tar/gzip/bzip2/xz/zstd/lz4/lzip/lzma 等主流格式

## 快速开始

```bash
# 安装依赖
sudo apt install unzip unrar 7zip tar gzip bzip2 xz-utils zstd lz4 lzip

# 编译
make

# 解压
uz archive.zip
uz -o dest archive.tar.gz
uz -l archive.7z
uz -t archive.zip

# 查看帮助
uz --help
```

## 许可证

本项目采用 [MIT](LICENSE) 许可。

- 作者：X2BAM2X
- 创建时间：2026 年 11 月
- AI 辅助生成：是

## 特性

- 按魔数自动识别压缩格式，不依赖扩展名
- 自动判断压缩 tar 流（按 `.tar.gz`、`.tgz`、`.tar.xz` 等扩展名）
- 目标目录不存在时自动创建
- 支持解压、列出内容、完整性测试
- 常见格式优先使用专用命令，未知格式回退给 7z
- 归档路径以 `-` 开头时自动加 `./`，避免被当成命令行选项
- 支持多卷压缩包：`.001` 分卷自动交给 7z 读取
- 支持密码解密（-p/--password）
- 不读配置文件、不写缓存、不联网
- `-v` 打印识别结果和实际执行的命令行

## 设计哲学

- 不实现压缩算法，只做格式识别和命令分派
- 不链接任何压缩库，压缩/解压交给系统已有工具
- 不隐藏底层工具的错误，`-q` 才统一静默
- 不引入配置文件和后台状态，一次调用只做一件事

## 支持的格式

| 格式 | 识别方式 | 调用的命令 |
| --- | --- | --- |
| zip | `PK\x03\x04` 等 | `unzip` |
| rar | `Rar!\x1a\x07` | `unrar`，其次 `unar`，最后 `7z` |
| 7z | `7z\xbc\xaf\x27\x1c` | `7z` / `7za` / `7zz` |
| tar | 偏移 257 处的 `ustar` | `tar` |
| gzip | `\x1f\x8b` | `tar`（压缩 tar）或 `gzip` |
| bzip2 | `BZh` | `tar`（压缩 tar）或 `bzip2` |
| xz | `\xfd7zXZ\x00` | `tar`（压缩 tar）或 `xz` |
| zstd | `\x28\xb5\x2f\xfd` | `tar`（压缩 tar）或 `zstd` |
| lz4 | `\x04\x22\x4d\x18` | `tar`（压缩 tar）或 `lz4` |
| compress | `\x1f\x9d` | `tar`（压缩 tar，经 `gzip`）或 `gzip` |
| lzip | `LZIP` | `tar`（压缩 tar）或 `lzip` |
| lzma | `\x5d` + 字典大小 | `tar`（压缩 tar）或 `xz` |
| 其他 | 无法识别 | 尝试 `7z` |

压缩 tar 通过扩展名判断（`.tar.gz`、`.tgz`、`.tar.xz` 等）。裸压缩流（单独的 `.gz`、`.bz2` 等）会解压成去掉扩展名后的文件。

## 依赖

`uz` 只是一个调度器，实际解压靠系统命令。不同发行版包名略有差异，按需安装即可：

Arch Linux：

```bash
sudo pacman -S unzip unrar 7zip tar gzip bzip2 xz zstd lz4 lzip
```

Debian / Ubuntu：

```bash
sudo apt update
sudo apt install unzip unrar 7zip tar gzip bzip2 xz-utils zstd lz4 lzip
```

较老的 Debian/Ubuntu 没有 `7zip` 包时，可改用 `p7zip-full`；`unrar` 位于 non-free/multiverse 仓库。

Fedora：

```bash
sudo dnf install unzip unrar 7zip tar gzip bzip2 xz zstd lz4 lzip
```

`unrar` 需要启用 RPM Fusion；较老的 Fedora 可将 `7zip` 换成 `p7zip p7zip-plugins`。

openSUSE：

```bash
sudo zypper install unzip unrar 7zip tar gzip bzip2 xz zstd lz4 lzip
```

`unrar` 可能需要启用 Packman 仓库。

Alpine Linux：

```bash
sudo apk add unzip unrar 7zip tar gzip bzip2 xz zstd lz4 lzip
```

没有对应命令时，相关格式会报错并指出缺失的命令；rar 在缺少 `unrar` 时会尝试 `unar` 或 `7z`。

## 编译

```bash
git clone https://github.com/X1BAM1X/unizip.git
cd unizip
make
```

生成可执行文件 `uz`。

编译参数默认是 `-std=c11 -O2 -Wall -Wextra -Wpedantic -Werror`，可通过 `CFLAGS` 覆盖：

```bash
make CFLAGS="-std=c11 -O2 -g"
```

静态链接：

```bash
make static
strip uz.static   # 可选
```

会生成 `uz.static`。glibc 静态链接体积较大；用 musl 工具链通常会小很多。构建过程不注入时间戳，源码相同的构建结果可复现。

## 安装

```bash
sudo make install
```

默认安装到 `/usr/local/bin/uz`，同时安装 man page 到 `/usr/local/share/man/man1/uz.1`。可用 `DESTDIR` 或修改 Makefile 调整。也可以直接把 `uz` 放进 `PATH` 里的任意目录。

卸载：

```bash
sudo make uninstall
```

## 用法

```text
uz [选项] <压缩包> [目标目录]
```

### 选项

| 选项 | 说明 |
| --- | --- |
| `-o, --output DIR` | 解压到指定目录（默认：压缩包旁边，与压缩包同名的目录） |
| `-f, --force` | 覆盖已存在的文件 |
| `-l, --list` | 列出压缩包内容 |
| `-t, --test` | 测试压缩包完整性 |
| `-p, --password STR` | 压缩包密码（适用于 zip、7z、rar） |
| `-q, --quiet` | 抑制外部命令输出 |
| `-v, --verbose` | 显示识别出的格式和实际执行的命令（密码显示为 `***`） |
| `-h, --help` | 显示帮助 |
| `-V, --version` | 显示版本 |

如果没有用 `-o` 或位置参数指定目标目录，容器格式会解压到 `<压缩包所在目录>/<压缩包名>/`；裸压缩流则直接写到压缩包所在目录下的单个文件。

### 退出码

`uz` 会把外部命令的返回值统一映射成自己的退出码：

| 退出码 | 含义 |
| --- | --- |
| 0 | 成功 |
| 1 | 参数或用法错误 |
| 2 | 解压、列表或完整性测试失败 |
| 3 | 缺少所需的系统命令 |
| 4 | 内部错误（内存、fork、I/O 等） |

因此脚本里不需要分别判断 `unzip` 返回 1、`tar` 返回 2、`unrar` 返回 3。

目标目录既可以放在压缩包后面，也可以用 `-o` 指定。

### 示例

```bash
# 解压到压缩包旁边，生成 package/
uz package.zip

# 解压到指定目录
uz package.zip out/
uz -o out package.tar.gz

# 覆盖已有文件
uz -f backup.7z restored/

# 查看内容
uz -l package.rar
uz -l archive.tar.xz

# 测试完整性
uz -t package.7z
uz -t archive.tar.zst

# 查看识别结果和实际执行的命令（未知格式会尝试交给 7z）
uz -v package.deb out/

# 安静解压
uz -q package.zip out/

# 密码解压
uz -p mypassword archive.zip
uz --password mypassword archive.7z
uz -p mypassword archive.rar

# 多卷压缩包自动读取
uz archive.zip.001 out/
uz archive.7z.001 out/
```

## 工作流程

1. 读取文件前 512 字节，根据魔数判断格式。
2. 如果是压缩流，根据扩展名判断它是否包裹了 tar。
3. 根据格式选择系统命令并构造参数。
4. 解压时按需创建目标目录；未指定目标时，默认使用压缩包旁边与压缩包同名的目录。
5. 对于裸压缩流，将解压结果写入目标目录下去掉压缩扩展名后的文件。
6. 对于 `.001` 多卷压缩包，自动检测并交给 7z 读取全部分卷。
7. 如果提供了密码，传递给解压命令进行解密。

整个程序不解析压缩包内部结构，也不自己实现解压算法，因此行为基本与系统命令一致。

## 项目结构

```text
unizip/
├── unizip.c     # CLI 入口与参数解析
├── unizip.h     # 公共类型、版本号
├── detect.c     # 格式识别
├── detect.h
├── extract.c    # 解压、列表、校验调度
├── extract.h
├── util.c       # 命令查找、进程执行、路径处理
├── util.h
├── Makefile
├── LICENSE
├── README.md          # English
├── README_ZH_CN.md    # 简体中文
├── CHANGELOG.md       # 版本历史
├── uz.1               # man page
├── .gitignore
└── examples/    # 示例脚本
    ├── basic-usage.sh
    ├── batch-extract.sh
    ├── find-archives.sh
    ├── check-integrity.sh
    └── extract-and-clean.sh
```

## 已知限制

- 裸压缩流没有多文件清单：`-l` 只输出解压后的文件名，`-t` 调用对应解压器做完整性校验。
- `-f` 影响 zip、7z、rar、unar 和裸压缩流；`tar` 的覆盖行为由它自己决定。
- 压缩 tar（包括 `.tar.Z`）通过 `tar -I` 解压，需要 GNU tar；BusyBox tar 未验证，可能不支持。
- 未提供 `-p` 时，rar 测试/列出/解压会给 `unrar` 传 `-p-`，遇到加密包直接失败而不是等待输入。
- `-t` 对 tar 系列只丢弃标准输出，因此解压器的错误默认可见；加 `-q` 才会一起静默。
- 压缩 tar 只按扩展名识别；改名成 `.gz` 后会被当作裸压缩流解压成 `.tar` 文件。
- 除密码外不接受交互式输入。
- 依赖系统命令，不同发行版上同名命令的参数兼容性可能略有差异。
- 对未知格式会尝试调用 7z，因此非压缩文件也会得到 7z 的报错信息。


## 故障排查

### 解压失败

```bash
# 查看详细错误
uz -v archive.zip 2>&1

# 检查依赖命令
which unzip 7z tar

# 查看错误码
uz archive.zip; echo $?
```

### 命令未找到

确保已安装对应工具：

```bash
# Debian/Ubuntu
sudo apt install unzip unrar 7zip tar gzip bzip2 xz-utils zstd lz4 lzip

# Arch Linux
sudo pacman -S unzip unrar 7zip tar gzip bzip2 xz zstd lz4 lzip

# Fedora
sudo dnf install unzip unrar 7zip tar gzip bzip2 xz zstd lz4 lzip
```

### 路径问题

文件名以 `-` 开头时需要特殊处理：

```bash
uz -o dest -- -hidden.tar.gz
```

## 贡献

欢迎提交 Issue 和 Pull Request。请查看 examples/ 目录获取使用示例。
