#define _GNU_SOURCE
#include <sched.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <stdio.h>
#include <stdlib.h>
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

static int container_func(void *hostname)
{

    pid_t pid = getpid();
    printf("Container[%d] - inside the container!\n", pid);

    // 使用 sethostname 设置子进程的 hostname 信息
    struct utsname uts;
    if (sethostname(hostname, strlen(hostname)) == -1) {
        errExit(-1, "sethostname")
    };

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

    if (argc < 2) {
        hostname = "container";
    } else {
        hostname = argv[1];
    }

    pid_t pid = getpid();

    printf("Parent[%d] - create a container!\n", pid);
    // 创建并启动子进程，调用该函数后，父进程将继续往后执行，也就是执行后面的waitpid
    pid_t child_pid = clone(container_func,  // 子进程将执行container_func这个函数
                    container_stack + sizeof(container_stack),
                    // CLONE_NEWUTS表示创建新的UTS namespace，
                    // 这里SIGCHLD是子进程退出后返回给父进程的信号，跟namespace无关
                    CLONE_NEWUTS | CLONE_NEWIPC | SIGCHLD,
                    hostname);  // 传给child_func的参数
    errExit(child_pid, "clone");

    waitpid(child_pid, NULL, 0); // 等待子进程结束

    printf("Parent[%d] - container exited!\n", pid);
    return 0;
}
