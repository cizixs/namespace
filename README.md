# namespace
Linux namespace demo scripts.

## Usage


### container.c

`container.c` is the main script that creates a child process running `/bin/bash` which aims to act like a container.

```
gcc container.c -o container
sudo ./container
```

### join_ns.c

`join_ns.c` is a utility script that makes current process join an existing namespace.

```
gcc join_ns.c -o join_ns
sudo ./join_ns /proc/[PID]/ns/[FILE]
```

NOTE: `join_ns.c` is copied from setns(2) man page example mainly, and a few edits are made.

## Changelog

- v0.2: create a child process with isolated UTS namespace
- v0.1: create a simple child process that shares namespaces with parent
