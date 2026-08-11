#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"


void find(const char *path, const char *filename)
{
    char buf[512];
    char *p;

    int fd;
    struct dirent de;
    struct stat st;


    // 打开当前路径
    if ((fd = open(path, 0)) < 0)
    {
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }


    // 获取文件信息
    if (fstat(fd, &st) < 0)
    {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }


    /*
     * 如果当前路径本身就是文件
     * 例如:
     * find ./a/b b
     */
    if (st.type == T_FILE)
    {
        char *name = (char *)path + strlen(path);

        while (name >= path && *name != '/')
            name--;

        name++;

        if (strcmp(name, filename) == 0)
        {
            printf("%s\n", path);
        }

        close(fd);
        return;
    }



    /*
     * 如果当前路径是目录
     */
    if (st.type != T_DIR)
    {
        close(fd);
        return;
    }


    strcpy(buf, path);


    while (read(fd, &de, sizeof(de)) == sizeof(de))
    {
        if (de.inum == 0)
            continue;


        // 跳过 "." 和 ".."
        if (strcmp(de.name, ".") == 0 ||
            strcmp(de.name, "..") == 0)
            continue;


        /*
         * 每次重新拼接路径
         *
         * 例如:
         * path="./a"
         * de.name="b"
         *
         * 得到:
         * ./a/b
         */
        p = buf + strlen(path);


        // 防止出现 //
        if (*(p - 1) != '/')
            *p++ = '/';


        strcpy(p, de.name);



        int child_fd = open(buf, 0);

        if (child_fd < 0)
            continue;


        if (fstat(child_fd, &st) < 0)
        {
            close(child_fd);
            continue;
        }



        if (st.type == T_FILE)
        {
            if (strcmp(de.name, filename) == 0)
            {
                printf("%s\n", buf);
            }
        }
        else if (st.type == T_DIR)
        {
            /*
             * 关闭当前打开的目录
             * 递归函数内部会重新 open
             */
            close(child_fd);

            find(buf, filename);

            continue;
        }


        close(child_fd);
    }


    close(fd);
}



int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(2, "find: missing argument\n");
        exit(1);
    }


    find(argv[1], argv[2]);


    exit(0);
}