#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#define LOOPNUM 5
 
void critical(int proc, int loop)
{
  int n = ((rand() % 200)+1)*10000;
  
  if (proc) {
    printf("Parent (%3d): Enter critical (sleep %d ms)\n", loop, n/1000);
  } else {
    printf("Child (%3d):  Enter critical (sleep %d ms)\n", loop, n/1000);
  }
  usleep(n);
  if (proc) {
    printf("Parent (%3d): Exit critical\n", loop);
  } else {
    printf("Child (%3d):  Exit critical\n", loop);
  }
}

void remain()
{
    int n = ((rand() % 400)+1)*10000;
//    int n = ((rand() % 100)+1)*10000;
  
  usleep(n);
}

// UNIX のセマフォを使って P命令を実現
void P(int semid)
{
  struct sembuf sops;
  
  sops.sem_num = 0;		// semaphore number
  sops.sem_op = -1;		// operation (decrement semaphore)
  sops.sem_flg = SEM_UNDO;	// The operarion is canceled when the calling porcess terminates
  if (semop(semid, &sops, 1)) {
    perror("semop");		// Fail operation to semaphore
    if (semctl(semid, 0, IPC_RMID) == -1) {	// remove semaphore
      perror("shmctl");
    }
    exit(1);
  }
}

// UNIX のセマフォを使って V命令を実現
void V(int semid)
{
  struct sembuf sops;
  
  sops.sem_num = 0;		// semaphore number
  sops.sem_op = 1;		// operation (increment semaphore)
  sops.sem_flg = SEM_UNDO;	// The operarion is canceled when the calling porcess terminates
  if (semop(semid, &sops, 1)) {
    perror("semop");		// Fail operation to semaphore
    if (semctl(semid, 0, IPC_RMID) == -1) {	// remove semaphore
      perror("shmctl");
    }
    exit(1);
  }
}

int main()
{
  int pid;
  int status;
  int loop;
  int semid;
  struct sembuf sops;
  int initval;
  int *x;

  // セマフォの作成
  semid = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
  if (semid == -1) {
    perror("semget");
    exit(1);
  }
  
  // セマフォ変数の初期化
  initval = 1;
  if (semctl(semid, 0, SETVAL, initval) == -1) {
    perror("semctl at initializing value of semaphore"); 
  }
  
  pid = fork();
  switch (pid) {
  case    0:
    printf("I am child process\n");
    for (loop = 0; loop < LOOPNUM; loop++) {
      printf("Child  (%3d): Require semaphore\n", loop);
      P(semid);
      critical(pid, loop);
      printf("Child  (%3d): Release semaphore\n", loop);
      V(semid);
      remain();
    }
    break;
  case    -1:
    printf("Fork error\n");
    exit(1);
  default:
    printf("Process id of child process is %d\n", pid);
    srand(pid);	/* randomize */
    for (loop = 0; loop < LOOPNUM; loop++) {
      printf("Parent (%3d): Require semaphore\n", loop);
      P(semid);
      critical(pid, loop);
      printf("Parent (%3d): Release semaphore\n", loop);
      V(semid);
      remain();
    }
  }

  if (pid != 0) {
    if (wait(&status) == -1) {
      perror("Wait error\n");
    } else {
      printf("Return value is %d\n",status);
    }
    // セマフォの解放
    if (semctl(semid, 0, IPC_RMID) == -1) {
      perror("shmctl");
    }
  } else {
    return 0;
  }
}
