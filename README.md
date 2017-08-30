# namespace
Linux namespace demo scripts. It is create to demostrate linux namespace usage while writing [this blog post](http://cizixs.com/2017/08/29/linux-namespace)(Chinese Version).

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

- v0.6: add User namespace support, ordinary user in host can run the process and become root in container
- v0.5: add IPC namespace, isolate inter process communication
- v0.4: add Mount namespace supprt, container have its own /proc and /tmp directory
- v0.3: add PID namespace support
- v0.2: create a child process with isolated UTS namespace
- v0.1: create a simple child process that shares namespaces with parent
