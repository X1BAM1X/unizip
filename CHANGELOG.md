# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.0] - 2026-11-20

First public release.

### Added
- Magic-based detection for zip, rar, 7z, tar, gzip, bzip2, xz, zstd, lz4, compress, lzip and lzma
- Compressed tar streams extracted through `tar -I`
- Extraction, listing (`-l`) and integrity testing (`-t`)
- rar fallback from `unrar` to `unar` and then 7z; unknown formats fall back to 7z
- Password support for zip, 7z and rar
- `.001` multi-volume archive support through 7z
- Unified exit codes: 0 ok, 1 usage, 2 failure, 3 missing tool, 4 internal
- `-v` prints the detected format and exact commands, with passwords masked
- `-f`, `-q`, `-o` and leading-dash path normalization
- Default extraction into a sibling directory named after the archive; bare streams stay beside the archive
- Dynamic path handling for long paths
- `make static` target
- `uz(1)` man page
- English and Simplified Chinese README
- MIT license

### Fixed
- Bare compressed streams no longer try to open the destination directory as a file
- `unzip -P` is placed before the archive name so encrypted zips extract correctly
- `unrar` receives the password as `-pPASSWORD`
- `uz_run_to_file` no longer leaks pipe descriptors or leaves children unreaped on failure
- Split archive detection requires a full three-digit `.NNN` suffix
- Split rar listing and testing go through 7z
- `-t` on tar streams keeps decompressor errors unless `-q` is given
- `unrar` gets `-p-` when no password is supplied
