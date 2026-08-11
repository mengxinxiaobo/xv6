#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/param.h"


int main(int argc, char *argv[])
{
    char buf[512];
    char *xargv[MAXARG];

    // 保存 xargs 后面的原始命令参数
    int xargc = 0;
    for(int i = 1; i < argc; i++)
    {
        xargv[xargc] = argv[i];
        xargc++;
    }


    char c;
    int index = 0;


    while(read(0, &c, 1) == 1)
    {
        // 读取一行结束
        if(c == '\n')
        {
            buf[index] = 0;


            // 当前行作为新的参数
            xargv[xargc] = buf;
            xargv[xargc + 1] = 0;


            int pid = fork();

            if(pid == 0)
            {
                exec(xargv[0], xargv);

                // exec失败
                fprintf(2, "xargs: exec failed\n");
                exit(1);
            }
            else if(pid > 0)
            {
                wait(0);
            }


            // 下一行重新读取
            index = 0;
        }
        else
        {
            buf[index++] = c;
        }
    }


    exit(0);
}