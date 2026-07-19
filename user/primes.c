#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// 素数筛选函数。
// left_read 是当前进程左侧管道的读端文件描述符。
void sieve(int left_read);

int
main(int argc, char *argv[])
{
    // primes 程序不需要额外的命令行参数。
    // 正常运行时 argc == 1，argv[0] 是程序名 "primes"。
    if(argc != 1){
        fprintf(2, "Usage: primes\n");
        exit(1);
    }

    // 创建第一条管道。
    //
    // first_pipe[0]：读端
    // first_pipe[1]：写端
    //
    // 原始父进程负责向写端写入 2～35；
    // 第一个筛选子进程负责从读端读取这些数字。
    int first_pipe[2];

    if(pipe(first_pipe) < 0){
        fprintf(2, "pipe error\n");
        exit(1);
    }

    // 创建第一个筛选进程。
    int pid = fork();

    if(pid == 0){
        // ---------------- 第一个筛选子进程 ----------------

        // 子进程只需要读取 first_pipe，
        // 不需要向 first_pipe 写入，所以关闭写端。
        close(first_pipe[1]);

        // 从第一条管道开始进行素数筛选。
        sieve(first_pipe[0]);


    }else if(pid > 0){
        // ---------------- 原始父进程：数字生成器 ----------------

        // 父进程只向 first_pipe 写入数据，
        // 不需要从中读取，所以关闭读端。
        close(first_pipe[0]);

        // 将 2～35 依次写入第一条管道。
        //
        // 这里直接传输 int 的二进制数据，
        // 每次写入 sizeof(int) 个字节。
        for(int i = 2; i <= 35; i++){
            write(first_pipe[1], &i, sizeof(int));
        }

        // 所有数字发送完成后，关闭管道写端。
        //
        // 当第一个筛选进程读完管道中的全部数据后，
        // 下一次 read() 会因为所有写端都已关闭而返回 0，
        // 从而知道上游已经不会再发送新数据。
        close(first_pipe[1]);

        // 等待第一个筛选子进程退出。
        //
        // 第一个筛选进程又会等待它自己的子进程，
        // 后面的进程依次等待各自的子进程，
        // 因此这里最终会间接等待整条流水线结束。
        wait(0);

        exit(0);

    }else{
        // fork() 返回负数，表示创建子进程失败。
        fprintf(2, "fork error\n");
        exit(1);
    }

    // 正常情况下不会执行到这里。
    exit(0);
}

void
sieve(int left_read)
{
    // prime 保存当前筛选进程所负责的素数。
    int prime;

    // 从左侧管道读取第一个整数。
    //
    // 当前管道中的第一个数字一定是本层尚未被过滤掉的
    // 最小数字，因此它一定是一个素数。
    int result = read(left_read, &prime, sizeof(prime));

    // read() 返回 0 表示：
    //
    // 1. 管道中已经没有数据；
    // 2. 管道的所有写端都已经关闭；
    // 3. 以后不会再有新数据。
    //
    // 当前进程没有数字需要处理，因此直接退出。
    if(result == 0){
        close(left_read);
        exit(0);
    }

    // 每次应该读取一个完整的 int。
    // 如果返回值不是 sizeof(prime)，说明读取失败
    // 或没有读到一个完整的整数。
    if(result != sizeof(prime)){
        fprintf(2, "read error\n");
        close(left_read);
        exit(1);
    }

    // 输出当前筛选进程发现的素数。
    printf("prime %d\n", prime);

    // 创建通往下一级筛选进程的管道。
    //
    // next_pipe[0]：下一级子进程的读端
    // next_pipe[1]：当前进程的写端
    int next_pipe[2];

    if(pipe(next_pipe) < 0){
        fprintf(2, "pipe error\n");
        close(left_read);
        exit(1);
    }

    // 创建下一级筛选进程。
    int pid = fork();

    if(pid == 0){
        // ---------------- 下一级筛选子进程 ----------------

        // left_read 属于当前这一层的输入管道。
        // 下一级子进程不再需要读取旧管道，所以关闭它。
        close(left_read);

        // 下一级子进程只从 next_pipe 中读取数据，
        // 不向 next_pipe 写入，所以关闭写端。
        close(next_pipe[1]);

        // 对过滤后的数字继续执行相同的筛选流程。
        //
        // 每次递归调用都会在一个新的子进程中执行，
        // 从而形成一条进程流水线。
        sieve(next_pipe[0]);

        // 正常情况下 sieve() 内部会退出。
        exit(0);

    }else if(pid > 0){
        // ---------------- 当前筛选进程 ----------------

        // 当前进程只向 next_pipe 写入过滤后的数字，
        // 不需要从 next_pipe 读取，所以关闭读端。
        close(next_pipe[0]);

        int num;

        // 前面已经读取了第一个数字 prime。
        // 这里继续读取左侧管道中的剩余整数。
        //
        // 只有每次成功读取一个完整的 int 时，
        // 循环才会继续。
        while((result = read(left_read, &num, sizeof(num)))
              == sizeof(num)){

            // 如果 num 不能被当前素数 prime 整除，
            // 说明它没有被本层过滤掉，需要传给下一层。
            //
            // 如果 num 是 prime 的倍数，则直接丢弃。
            if(num % prime != 0){
                // 将保留下来的整数写入下一条管道。
                if(write(next_pipe[1], &num, sizeof(num))
                   != sizeof(num)){
                    fprintf(2, "write error\n");

                    // 写入失败时关闭当前进程持有的文件描述符。
                    close(left_read);
                    close(next_pipe[1]);
                    exit(1);
                }
            }
        }

        // result < 0 表示 read() 发生错误。
        //
        // 正常读取结束时，result 应该为 0，
        // 表示左侧管道已经到达 EOF。
        if(result < 0){
            fprintf(2, "read error\n");
            close(left_read);
            close(next_pipe[1]);
            exit(1);
        }

        // 当前进程已经读取并处理完左侧管道的所有数据，
        // 因此关闭左侧管道读端。
        close(left_read);

        // 当前进程已经把所有保留下来的数字传给了下一层，
        // 以后不会再向 next_pipe 写入数据，所以关闭写端。
        //
        // 当下一级进程读完管道中已有的数据后，
        // 它的下一次 read() 会返回 0，从而知道输入结束。
        close(next_pipe[1]);

        // 等待下一级筛选子进程退出。
        //
        // 下一级进程又会等待它自己的子进程，
        // 因此退出过程会从流水线最右侧逐层向左返回。
        wait(0);

        exit(0);

    }else{
        // fork() 返回负数，表示创建下一级筛选进程失败。
        fprintf(2, "fork error\n");

        // 关闭当前进程持有的所有相关文件描述符。
        close(left_read);
        close(next_pipe[0]);
        close(next_pipe[1]);

        exit(1);
    }
}