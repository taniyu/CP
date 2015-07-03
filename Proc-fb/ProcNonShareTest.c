#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define LOOPNUM 10

int x;

int main()
{
  int pid;
  int status;
  int i;

  pid = fork();
  switch (pid) {
  case 0:
    printf("I am child process\n");
    printf("Address  of x is %p\n", &x);
    break;
  case -1:
    printf("Fork error\n");
    exit(1);
  default:
    printf("Process id of child process is %d\n",pid);
    printf("Address  of x is %p\n", &x);
  }
  // 親プロセスと子プロセスで同時に実行する．pid の値で親子を識別している．
  for (i = 0; i < LOOPNUM; i++) {
    x++;
    printf("%s: x = %d\n", (pid) ? "Parent" : "Child ", x);
    usleep(((pid ? 1 : 0) + 1)*500000);
  }
  if (pid != 0) {
    if (wait(&status) == -1) {
      perror("Wait error\n");
    } else {
      printf("Return value is %d\n",status);
    }
  } else {
    return 0;
  }
}
