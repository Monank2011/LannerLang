# Lanner DevOps and Scripting Runtime

Lanner can build small command-line utilities and compiled scripts without a separate interpreter runtime. `lanner --script file.lan -- args...` compiles a temporary native executable, forwards the arguments after `--`, runs it, and removes the temporary executable. `--run` supports the same argument forwarding.

## Command arguments

```lanner
Args.count()
Args.at(1)
```

The generated program receives its real process argument vector, so argument parsing remains ordinary Lanner code.

## Environment

```lanner
Env.has("HOME")
Env.get("HOME")
Env.set("MODE", "prod")
Env.unset("MODE")
```

## Filesystem and paths

`FS` provides existence/type tests, byte-buffer reads, text/buffer writes, append, remove, directory creation/removal, rename, copy, current-directory queries, directory changes, and newline-delimited directory listing. `Path` provides join, basename, dirname, extension, stem, normalization, absolute-path construction, and absolute-path testing.

Path helper results use thread-local scratch storage. Copy the value into an owned Lanner string before making another path-helper call when longer-lived storage is needed.

## Regex

`Regex.compile`, `Regex.isMatch`, `Regex.find`, and `Regex.free` provide a small deterministic regex engine suitable for CLI validation and text processing. The supported syntax includes literals, `.`, character classes such as `[a-z]`, negated classes, `^`/`$` anchors, escaping, and `*`, `+`, `?` quantifiers. Invalid patterns return an invalid regex handle; callers should keep patterns within the documented subset.

## Shell/process helpers

`Shell.run` executes a command and returns its exit status. `Shell.output` captures stdout into an owned `Buffer`. `Shell.which` resolves an executable through the host `PATH`. For explicit process lifecycle control use `Process.spawn`, `Process.wait`, `Process.pid`, and `Process.terminate`.

These APIs are intentionally explicit hosted operations. A freestanding Lanner program does not link the hosted runtime.
