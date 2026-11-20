# unizip (uz)

English | [简体中文](README_ZH_CN.md)

## Why this project

Have you ever run into this?

```bash
# zip needs unzip
unzip archive.zip

# rar needs unrar
unrar l archive.rar

# 7z needs 7z
7z l archive.7z

# tar.gz needs tar
tar -tzf archive.tar.gz

# and then gzip, bzip2, xz, zstd...
```

Every archive format comes with its own command and its own syntax:

- `unzip` uses `-o`, `-f`, `-l`, and friends
- `unrar` is a different story
- `7z` has yet another set of switches
- `tar` variants have their own conventions

The annoying part is not extracting a single file. It is having to identify the format first, then remember the right command and the right flags every time.

## Why uz

`uz` is a unified front end for archives:

```bash
uz archive.zip      # extract a zip
uz archive.rar      # extract a rar
uz archive.7z       # extract a 7z
uz archive.tar.gz   # extract a tar.gz
```

**✓ One syntax** — the same command and options for every supported format.

**✓ Automatic detection** — the format is detected from magic bytes, not from the file extension.

**✓ No library dependencies** — written in plain C, linked against no compression library, and dispatches to the tools already installed on the system.

**✓ Password support** — pass `-p` for encrypted zip, 7z, and rar archives.

**✓ Broad format coverage** — zip, rar, 7z, tar, gzip, bzip2, xz, zstd, lz4, lzip, lzma, compress (`.Z`), and tar streams wrapped in any of them.

## Quick start

```bash
# Install the external tools you need
sudo apt install unzip unrar 7zip tar gzip bzip2 xz-utils zstd lz4 lzip

# Build
make

# Extract
uz archive.zip
uz -o dest archive.tar.gz
uz -l archive.7z
uz -t archive.zip

# Help
uz --help
```

## Features

- Detects formats from magic bytes, not from extensions
- Recognizes compressed tar streams by suffix (`.tar.gz`, `.tgz`, `.tar.xz`, and so on)
- Creates the destination directory when needed
- Supports extraction, listing, and integrity testing
- Uses native tools where possible, and falls back to 7z for unknown formats
- Prefixes archive paths starting with `-` with `./` so they are not parsed as options
- Reads `.001` multi-volume archives through 7z
- Supports passwords (`-p` / `--password`)
- Reads no configuration files, keeps no cache, and makes no network access
- `-v` prints the detected format and the exact command lines executed

## Design philosophy

- Do not implement compression algorithms; only detect formats and dispatch commands
- Do not link against compression libraries; use the tools the system already has
- Do not hide errors from the underlying tools; only `-q` silences them
- Do not introduce configuration files or background state; one invocation does one thing

## Supported formats

| Format | Detection | Command |
| --- | --- | --- |
| zip | `PK\x03\x04` and related signatures | `unzip` |
| rar | `Rar!\x1a\x07` | `unrar`, then `unar`, then `7z` |
| 7z | `7z\xbc\xaf\x27\x1c` | `7z` / `7za` / `7zz` |
| tar | `ustar` at offset 257 | `tar` |
| gzip | `\x1f\x8b` | `tar` (compressed tar) or `gzip` |
| bzip2 | `BZh` | `tar` (compressed tar) or `bzip2` |
| xz | `\xfd7zXZ\x00` | `tar` (compressed tar) or `xz` |
| zstd | `\x28\xb5\x2f\xfd` | `tar` (compressed tar) or `zstd` |
| lz4 | `\x04\x22\x4d\x18` | `tar` (compressed tar) or `lz4` |
| compress | `\x1f\x9d` | `tar` (compressed tar, through `gzip`) or `gzip` |
| lzip | `LZIP` | `tar` (compressed tar) or `lzip` |
| lzma | `\x5d` + dictionary size | `tar` (compressed tar) or `xz` |
| other | unrecognized | try `7z` |

Compressed tar streams are recognized by suffix (`.tar.gz`, `.tgz`, `.tar.xz`, and so on). A bare compressed stream such as a standalone `.gz` file is decompressed to a single file with the compression suffix removed.

## Dependencies

`uz` is only a dispatcher. Extraction is performed by system commands. Package names vary slightly between distributions, so install only the tools you actually need:

Arch Linux:

```bash
sudo pacman -S unzip unrar 7zip tar gzip bzip2 xz zstd lz4 lzip
```

Debian / Ubuntu:

```bash
sudo apt update
sudo apt install unzip unrar 7zip tar gzip bzip2 xz-utils zstd lz4 lzip
```

Older Debian/Ubuntu releases may not have a `7zip` package; use `p7zip-full` instead. `unrar` lives in the non-free/multiverse repositories.

Fedora:

```bash
sudo dnf install unzip unrar 7zip tar gzip bzip2 xz zstd lz4 lzip
```

`unrar` requires RPM Fusion. On older Fedora releases, replace `7zip` with `p7zip p7zip-plugins`.

openSUSE:

```bash
sudo zypper install unzip unrar 7zip tar gzip bzip2 xz zstd lz4 lzip
```

`unrar` may require the Packman repository.

Alpine Linux:

```bash
sudo apk add unzip unrar 7zip tar gzip bzip2 xz zstd lz4 lzip
```

If a command is missing, the corresponding format reports the missing command. For rar, `uz` tries `unar` and then `7z` when `unrar` is unavailable.

## Building

```bash
git clone https://github.com/X1BAM1X/unizip.git
cd unizip
make
```

This produces the `uz` executable.

The default compiler flags are `-std=c11 -O2 -Wall -Wextra -Wpedantic -Werror`. You can override them:

```bash
make CFLAGS="-std=c11 -O2 -g"
```

Static build:

```bash
make static
strip uz.static   # optional
```

This produces `uz.static`. A glibc static binary is fairly large; a musl toolchain usually produces a much smaller one. The build injects no timestamps, so identical sources produce reproducible results.

## Installation

```bash
sudo make install
```

By default this installs `/usr/local/bin/uz` and the man page at `/usr/local/share/man/man1/uz.1`. Use `DESTDIR` or edit the Makefile to change the locations. You can also copy `uz` into any directory on your `PATH`.

Uninstall:

```bash
sudo make uninstall
```

## Usage

```text
uz [options] <archive> [destination]
```

### Options

| Option | Description |
| --- | --- |
| `-o, --output DIR` | Extract into `DIR` (default: a directory named after the archive, next to it) |
| `-f, --force` | Overwrite existing files |
| `-l, --list` | List archive contents |
| `-t, --test` | Test archive integrity |
| `-p, --password STR` | Archive password (zip, 7z, rar) |
| `-q, --quiet` | Suppress output from external commands |
| `-v, --verbose` | Show the detected format and executed commands (passwords are shown as `***`) |
| `-h, --help` | Show help |
| `-V, --version` | Show version |

Without `-o` or a positional destination, container archives are extracted into `<archive-directory>/<archive-name>/`. Bare compressed streams are written into the archive's directory as a single file.

### Exit status

`uz` maps the return values of external commands to its own exit codes:

| Code | Meaning |
| --- | --- |
| 0 | Success |
| 1 | Usage or argument error |
| 2 | Extraction, listing, or integrity test failed |
| 3 | A required system command is missing |
| 4 | Internal error (memory, fork, I/O, and so on) |

Scripts no longer need to distinguish `unzip` returning 1, `tar` returning 2, and `unrar` returning 3.

The destination can be given either as the second positional argument or with `-o`.

### Examples

```bash
# Extract next to the archive, into package/
uz package.zip

# Extract into a specific directory
uz package.zip out/
uz -o out package.tar.gz

# Overwrite existing files
uz -f backup.7z restored/

# List contents
uz -l package.rar
uz -l archive.tar.xz

# Test integrity
uz -t package.7z
uz -t archive.tar.zst

# Show detection result and exact commands (unknown formats fall back to 7z)
uz -v package.deb out/

# Quiet extraction
uz -q package.zip out/

# Encrypted archives
uz -p mypassword archive.zip
uz --password mypassword archive.7z
uz -p mypassword archive.rar

# Multi-volume archives
uz archive.zip.001 out/
uz archive.7z.001 out/
```

## How it works

1. Read the first 512 bytes and detect the format from its magic bytes.
2. For compressed streams, decide from the suffix whether the stream wraps a tar archive.
3. Pick the system command for the format and build its argument list.
4. Create the destination directory when extracting; without an explicit destination, use a sibling directory named after the archive.
5. For a bare compressed stream, write the decompressed data to a file with the compression suffix removed.
6. For `.001` multi-volume archives, detect them and hand all volumes to 7z.
7. Pass the password to the extraction command when one is supplied.

`uz` does not parse archive internals and does not implement decompression itself, so its behavior largely matches that of the underlying tools.

## Project layout

```text
unizip/
├── unizip.c          # CLI entry point and argument parsing
├── unizip.h          # shared types and version
├── detect.c          # format detection
├── detect.h
├── extract.c         # extraction, listing, and test dispatch
├── extract.h
├── util.c            # command lookup, process execution, path helpers
├── util.h
├── Makefile
├── LICENSE
├── README.md         # English
├── README_ZH_CN.md   # Simplified Chinese
├── CHANGELOG.md      # version history
├── uz.1              # man page
├── .gitignore
└── examples/         # example scripts
    ├── basic-usage.sh
    ├── batch-extract.sh
    ├── find-archives.sh
    ├── check-integrity.sh
    └── extract-and-clean.sh
```

## Known limitations

- Bare compressed streams have no multi-file listing: `-l` prints the output filename, and `-t` asks the matching decompressor to validate the stream.
- `-f` affects zip, 7z, rar, unar, and bare streams; `tar` decides its own overwrite behavior.
- Compressed tar streams (including `.tar.Z`) are extracted through `tar -I`, which requires GNU tar. BusyBox tar is untested and may not work.
- Without `-p`, rar listing, testing, and extraction pass `-p-` to `unrar`, so encrypted archives fail immediately instead of waiting for input.
- `-t` on tar streams discards only stdout, so decompressor errors remain visible by default; `-q` silences them as well.
- Compressed tar streams are detected by suffix only. Renaming one to `.gz` makes it a bare compressed stream that decompresses to a `.tar` file.
- No interactive input other than passwords.
- Behavior depends on system commands; argument compatibility may vary between distributions.
- Unknown formats fall back to 7z, so non-archive files also produce 7z error messages.

## Troubleshooting

### Extraction fails

```bash
# Show detailed errors
uz -v archive.zip 2>&1

# Check for required commands
which unzip 7z tar

# Check the exit code
uz archive.zip; echo $?
```

### Command not found

Install the corresponding tool:

```bash
# Debian/Ubuntu
sudo apt install unzip unrar 7zip tar gzip bzip2 xz-utils zstd lz4 lzip

# Arch Linux
sudo pacman -S unzip unrar 7zip tar gzip bzip2 xz zstd lz4 lzip

# Fedora
sudo dnf install unzip unrar 7zip tar gzip bzip2 xz zstd lz4 lzip
```

### Paths starting with a hyphen

Use `--` to stop option parsing:

```bash
uz -o dest -- -hidden.tar.gz
```

## Contributing

Issues and pull requests are welcome. See the `examples/` directory for usage examples.

## License

This project is licensed under the [MIT License](LICENSE).

- Author: X2BAM2X
- Created: November 2026
- AI-assisted: yes
