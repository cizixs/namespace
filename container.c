#define _GNU_SOURCE
#include <sched.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <sys/utsname.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>

// 设置子进程要使用的栈空间
#define STACK_SIZE (1024*1024)
static char container_stack[STACK_SIZE];

#define errExit(code, msg); {if(code == -1){perror(msg); exit(-1);} }


char* const container_args[] = {
    "/bin/bash",
    NULL
};

struct child_args {
    char *hostname;  /*container hostname to use */
    int pipe_fd[2];
};

void update_map(char *file, int inside_id, int outside_id, int len) {
    FILE *mapfd = fopen(file, "w");
    if (mapfd == NULL) {
        errExit(-1, "open user map");
    }
    fprintf(mapfd, "%d %d %d", inside_id, outside_id, len);
    fclose(mapfd);
}

void update_uid_map(pid_t pid, int inside_id, int outside_id, int len) {
    char map_path[PATH_MAX];
    sprintf(map_path, "/proc/%ld/uid_map", (long) pid);
    update_map(map_path, inside_id, outside_id, len);
}

void update_gid_map(pid_t pid, int inside_id, int outside_id, int len) {
    char map_path[PATH_MAX];
    sprintf(map_path, "/proc/%ld/gid_map", (long) pid);
    update_map(map_path, inside_id, outside_id, len);
}

static int container_func(void *arg)
{
    struct child_args *args = (struct child_args *) arg;
    char *hostname = args->hostname;

    // wait for parent has updated UID and GID mapping
    char ch;
    close(args->pipe_fd[1]);
    if (read(args->pipe_fd[0], &ch, 1) != 0) {
        errExit(-1, "read pipe");
    }

    pid_t pid = getpid();
    printf("Container[%d] - inside the container!\n", pid);

    // 使用 sethostname 设置子进程的 hostname 信息
    struct utsname uts;
    if (sethostname(hostname, strlen(hostname)) == -1) {
        errExit(-1, "sethostname");
    };

    // mount /proc directory 
    system("mount --make-private /proc");
    if (mount("proc", "/proc", "proc", 0, NULL) != 0) {
        errExit(-1, "proc");
    };

    system("mount --make-private /tmp");
    if (mount("tmpfs", "/tmp", "tmpfs", 0, NULL) != 0){
        errExit(-1, "tmp");
    };

    // 使用 uname 获取子进程的机器信息，并打印 hostname 出来
    if (uname(&uts) == -1){
        errExit(-1, "uname");
    }
    printf("Container[%d] - container uts.nodename: [%s]!\n", pid, uts.nodename);

    // 用一个新的bash来替换掉当前子进程，
    // 这样我们就能通过 bash 查看当前子进程的情况.
    // bash退出后，子进程执行完毕
    execv(container_args[0], container_args);

    // 从这里开始的代码将不会被执行到，因为当前子进程已经被上面的bash替换掉了;
    // 所以如果执行到这里，一定是出错了
    printf("Container[%d] - oops!\n", pid);
    return 1;
}

int main(int argc, char *argv[])
{
    char *hostname;
    struct child_args args;

    if (argc < 2) {
        hostname = "container";
    } else {
        hostname = argv[1];
    }

    args.hostname = hostname;
    if (pipe(args.pipe_fd) == -1) {
        errExit(-1, "pipe");
    }

    // current process pid
    pid_t pid = getpid();

    // 打印父进程的 hostname 信息
    struct utsname uts;
    if (uname(&uts) == -1){
        errExit(-1, "uname")
    }
    printf("Parent[%d] - parent uts.nodename: [%s]!\n", pid, uts.nodename);

    // 创建并启动子进程，调用该函数后，父进程将继续往后执行，也就是执行后面的waitpid
    pid_t child_pid = clone(container_func,  // 子进程将执行container_func这个函数
                    container_stack + sizeof(container_stack),
                    // CLONE_NEWUTS表示创建新的UTS namespace，
                    // 这里SIGCHLD是子进程退出后返回给父进程的信号，跟namespace无关
                    CLONE_NEWUTS | CLONE_NEWPID | CLONE_NEWNS | CLONE_NEWIPC | CLONE_NEWUSER | SIGCHLD,
                    &args);  // 传给child_func的参数
    errExit(child_pid, "clone");

    // Update UID and GID map in the child
    const int uid=getuid(), gid=getgid();
    update_uid_map(child_pid, 0, uid, 1);
    update_gid_map(child_pid, 0, gid, 1);
    printf("Parent[%d] - user and group id mapping done...", pid);
    close(args.pipe_fd[1]);

    waitpid(child_pid, NULL, 0); // 等待子进程结束

    printf("Parent[%d] - container exited!\n", pid);
    return 0;
}
