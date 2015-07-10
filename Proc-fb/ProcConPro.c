#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define	STEP   100000 // 10万回のインクリメント毎に表示 
#define	TOTAL 1000000 // 全体で100万回インクリメントする 

void P(int semid)
{
  struct sembuf sops;
  
  sops.sem_num = 0;		// semaphore number
  sops.sem_op = -1;		// operation (decrement semaphore)
  sops.sem_flg = SEM_UNDO;	// The operarion is canceled when the calling porcess terminates
  semop(semid, &sops, 1);	// Omitt error handling
}

void V(int semid)
{
  struct sembuf sops;
  
  sops.sem_num = 0;		// semaphore number
  sops.sem_op = 1;		// operation (increment semaphore)
  sops.sem_flg = SEM_UNDO;	// The operarion is canceled when the calling porcess terminates
  semop(semid, &sops, 1);	// Omitt error handling
}

int main()
{
  int pid;
  int status;
  int loop1, loop2;
  int shmid;
  int *x;
  int semid;
  struct sembuf sops;
  
  shmid = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
  if (shmid == -1) {
    perror("shmget");
    exit(1);
  }
  x = (int *)shmat(shmid, NULL, 0);
  if (x == (int *)-1) {
    perror("shmat");
    exit(1);
  }
  *x = 0;

  semid = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
  if (semid == -1) {
    perror("semget");
    exit(1);
  }
  if (semctl(semid, 0, SETVAL, 1) == -1) {
    perror("semctl at initializing value of semaphore"); 
  }

  pid = fork();
  switch (pid) {
  case    0:
    printf("I am child process\n");
    break;
  case    -1:
    printf("Fork error\n");
    exit(1);
  default:
    printf("Process id of child process is %d\n",pid);
  }
  printf("%s: allocated address is %p\n", (pid) ? "Parent" : "Child ",x);
  for (loop1 = 0; loop1 < TOTAL; loop1 += STEP) {
    for (loop2 = 0; loop2 < STEP; loop2++) {
      P(semid);
      (*x)++;
      V(semid);
    }
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
    if (semctl(semid, 0, IPC_RMID)) {
      perror("semctl");
    }
  } else {
    return 0;
  }
}
