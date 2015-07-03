#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define	STEP   10000000	// 1千万回のインクリメント毎に表示 
#define	TOTAL 100000000	// 全体で1億回インクリメントする

int main()
{
  int pid;
  int status;
  int loop1, loop2;
  int shmid;
  volatile int *x;

  shmid = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
  if (shmid == -1) {
    perror("shmget");
    exit(1);
  }
  x = (int *)shmat(shmid, NULL, 0);	// 今回は fork前に共有メモリ空間を仮想空間に貼り付けてみる
  if (x == (int *)-1) {
    perror("shmat");
    exit(1);
  }
  *x = 0;
  pid = fork();
  switch (pid) {
  case 0:
    printf("I am child process\n");
    break;
  case -1:
    printf("Fork error\n");
    exit(1);
  default:
    printf("Process id of child process is %d\n",pid);
  }
  printf("%s: allocated address is %p\n", (pid) ? "Parent" : "Child ",x);
  for (loop1 = 0; loop1 < TOTAL; loop1 += STEP) {
    for (loop2 = 0; loop2 < STEP; loop2++) (*x)++;
    printf("%s (%8d): x = %d\n", (pid) ? "Parent" : "Child ", loop1+loop2, *x);
  }
  if (pid != 0) {
    if (wait(&status) == -1) {
      perror("Wait error\n");
    } else {
      printf("Return value is %d\n",status);
    }
    if (shmctl(shmid, IPC_RMID, NULL)) {
      perror("shmctl");
    }
  } else {
    return 0;
  }
}
