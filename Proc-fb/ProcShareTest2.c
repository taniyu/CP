#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define LOOPNUM 10

int main()
{
  int pid;
  int status;
  int i;
  int shmid;
  int *x;

  shmid = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
  if (shmid == -1) {
    perror("shmget");
    exit(1);
  }
  pid = fork();
  switch (pid) {
  case 0:
    printf("I am child process\n");
    x = (int *)shmat(shmid, (void *)0x40008000,  SHM_RND);	// アドレスを指定して割り付ける
    if (x == (int *)-1) {
      perror("shmat");
      exit(1);
    }
    break;
  case -1:
    printf("Fork error\n");
    exit(1);
  default:
    printf("Process id of child process is %d\n",pid);
    x = (int *)shmat(shmid, (void *)0x40004000,  SHM_RND);	// アドレスを指定して割り付ける
    if (x == (int *)-1) {
      perror("shmat");
      exit(1);
    }
  }

  printf("%s: allocated address is %p\n", (pid) ? "Parent" : "Child ",x);
  for (i = 0; i < LOOPNUM; i++) {
    (*x)++;
    printf("%s: x = %d\n", (pid) ? "Parent" : "Child ", *x);
    usleep(((pid ? 1 : 0) + 1)*500000);
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
