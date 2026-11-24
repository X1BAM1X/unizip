# Security notes

## Passwords

`uz` does not store passwords. It passes them to external tools as command-line
arguments:

```text
unzip -P PASSWORD ...
7z -pPASSWORD ...
unrar -pPASSWORD ...
unar -p PASSWORD ...
```

This means a password may be visible to other processes of the same user
through process listings or `/proc/<pid>/cmdline`, and it may end up in shell
history. Verbose mode (`-v`) masks passwords as `***`, but that only affects
`uz`'s own output.

To keep passwords out of shell history:

```bash
read -rsp 'Password: ' PW; echo
uz -p "$PW" archive.zip
unset PW
```

If the password must not appear in process arguments either, avoid passing it
on the command line and use an external wrapper that talks to the tool through
a pty. `uz` does not implement that because the supported tools handle prompts
differently.

Child standard input is connected to `/dev/null`. Encrypted archives without
`-p` therefore fail instead of waiting for interactive input.

## Untrusted archives

`uz` delegates extraction to system tools and does not implement a sandbox.
Archive contents, path traversal, symlinks, and decompression vulnerabilities
are handled by the underlying tools and their versions.

Do not extract untrusted archives as root. If isolation is required, run the
whole operation inside a container, VM, or a dedicated unprivileged user.

## Reporting

Please report security issues privately through GitHub's security advisory
feature, or contact the maintainer without publishing exploit details in a
public issue.
