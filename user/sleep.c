#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
int
main(int argc,char *argv[])
{
  if(argc!=2){
    fprintf(2,"Usage: sleep ticks\n");
    exit(1);
  }

//   sleep(atoi(argv[1]));
// atoi就是下面这行代码，下面的代码就是表示atoi进行字符串转整数
  int n;
  n = 0;
  char *s = argv[1];
  while('0' <= *s && *s <= '9')
    n = n*10 + *s++ - '0';
  sleep(n);
  exit(0);
}
