#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    // pingpong 程序不需要命令行参数。
    // argc 应该为 1，其中 argv[0] 是程序名 "pingpong"。
    if(argc != 1){
        fprintf(2, "Usage: pingpong\n");
        exit(1);
    }

    // 父进程向子进程发送数据的管道。
    // parent_to_child[0]：读端
    // parent_to_child[1]：写端
    int parent_to_child[2];

    // 子进程向父进程返回数据的管道。
    // child_to_parent[0]：读端
    // child_to_parent[1]：写端
    int child_to_parent[2];

    // 创建两个管道，分别负责两个方向的数据传输。
    int p1 = pipe(parent_to_child);
    int p2 = pipe(child_to_parent);

    // pipe() 返回负数表示管道创建失败。
    if(p1 < 0 || p2 < 0){
        fprintf(2, "pipe error\n");
        exit(1);
    }

    // 创建子进程。
    // 子进程中 fork() 返回 0。
    // 父进程中 fork() 返回子进程的 PID。
    // 返回负数表示创建失败。
    int pid = fork();

    if(pid == 0){
        // ---------------- 子进程 ----------------

        // 子进程只需要：
        // 1. 从 parent_to_child 管道读取数据
        // 2. 向 child_to_parent 管道写入数据
        //
        // 因此关闭不使用的两个管道端。

        // 子进程不向 parent_to_child 管道写数据。关闭写端可以防止子进程意外写入数据。
        close(parent_to_child[1]);

        // 子进程不从 child_to_parent 管道读数据。关闭读端可以防止子进程意外读取数据。
        close(child_to_parent[0]);

        // 用于保存从父进程接收到的一个字节。
        char buf[1];

        // 从父进程读取一个字节。
        // 如果管道中暂时没有数据，read() 会阻塞等待。
        read(parent_to_child[0], buf, 1);

        // 打印当前子进程的 PID 和收到的消息。
        printf("%d: received ping\n", getpid());

        // 将刚刚收到的字节通过另一个管道写回父进程。
        write(child_to_parent[1], buf, 1);

        // 数据传输完成，关闭子进程使用过的管道端。
        close(parent_to_child[0]);
        close(child_to_parent[1]);

    }else if(pid > 0){
        // ---------------- 父进程 ----------------

        // 父进程只需要：
        // 1. 向 parent_to_child 管道写入数据
        // 2. 从 child_to_parent 管道读取数据
        //
        // 因此关闭不使用的两个管道端。

        // 父进程不从 parent_to_child 管道读数据。关闭读端可以防止父进程意外读取数据。
        close(parent_to_child[0]);

        // 父进程不向 child_to_parent 管道写数据。
        close(child_to_parent[1]);

        // 向子进程发送一个字节。
        write(parent_to_child[1], "a", 1);

        // 用于保存子进程返回的一个字节。
        char buf[1];

        // 等待并读取子进程返回的字节。
        read(child_to_parent[0], buf, 1);

        // 打印当前父进程的 PID 和收到的消息。
        printf("%d: received pong\n", getpid());

        // 等待子进程退出，回收子进程资源，
        // 防止子进程成为僵尸进程。
        wait(0);

        // 数据传输完成，关闭父进程使用过的管道端。
        close(parent_to_child[1]);
        close(child_to_parent[0]);

    }else{
        // fork() 返回负数，表示子进程创建失败。
        fprintf(2, "fork error\n");
        exit(1);
    }

    // 父进程和子进程最终都会执行到这里并正常退出。
    exit(0);
}