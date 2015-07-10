#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <time.h>

#define	STEP   100000 // 10万回のインクリメント毎に表示 
#define	TOTAL 1000000 // 全体で100万回インクリメントする 
#define MAX_PID 100  // プロセスの最大数
#define QSIZE  100  // メッセージキュー領域の大きさ

// キューの宣言
typedef struct {
  int wptr;  // 入力位置
  int rptr;   // 出力位置
  int buff[QSIZE]; // データ格納用配列
  int size;   // データ数
} _queue;


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

// データをキューに書き込む 入力側の処理
void write_data(int l, int semid, _queue *x)
{
  int count = 0;
  int data, sleep_time;
  while ( 1 ) {
    if ( count > l ) { return; }
    data = rand() / (RAND_MAX + 1.0) * 61 + 20;
    P(semid);
    if ( x->size < QSIZE ) {
      x->buff[x->wptr++] = data;
      x->wptr %= l;
      x->size++;
      count++;
    }
    V(semid);
    printf("write size: %d %d", x->size, data);
    sleep_time = rand() / (RAND_MAX + 1.0) * 61 + 20;
    printf(", %d\n", sleep_time);
    usleep(sleep_time);
  }
}

// データをキューから読み出す 出力側の処理
void read_data(int m, int semid, _queue *x)
{
  int count = 0;
  int data;
  while ( 1 ) {
    if ( count > m ) { return; }
    P(semid);
    if ( x->size > 0 ) {
      data = x->buff[x->rptr++];
      x->rptr %= m;
      x->size--;
      count++;
    }
    V(semid);
    printf("read size: %d", x->size);
    printf(", %d\n", data);
    usleep(data);
  }
}

int main()
{
  int pid;
  int input_pids[MAX_PID];
  int output_pids[MAX_PID];
  int status;
  int loop1, loop2;
  int shmid;
  /* int *x; */
  _queue *x;
  int semid;
  struct sembuf sops;
  int pno;
  time_t t;
  srand((unsigned int)time(&t));

  shmid = shmget(IPC_PRIVATE, sizeof(_queue), IPC_CREAT | 0666);
  if (shmid == -1) {
    perror("shmget");
    exit(1);
  }
  x = (_queue *)shmat(shmid, NULL, 0);
  if (x == (_queue *)-1) {
    perror("shmat");
    exit(1);
  }
  // キューの初期化
  x->size = 0;
  x->wptr = 0;
  x->rptr = 0;

  semid = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
  if (semid == -1) {
    perror("semget");
    exit(1);
  }
  if (semctl(semid, 0, SETVAL, 1) == -1) {
    perror("semctl at initializing value of semaphore"); 
  }

  // 入力用プロセス生成
  for ( pno = 0; pno < 1; pno++ ) {
    input_pids[pno] = fork();
    if ( input_pids[pno] == -1 ) {
      break;
    } else if (input_pids[pno] == 0) {
      // 入力プロセスの処理
      write_data(10, semid, x);
      /* receiver(msgid, pno); */
      return pno;
    } else {
      printf("Process id of input child process is %d\n",input_pids[pno]);
    }      
  }

  // 入力用プロセス生成
  for ( pno = 0; pno < 1; pno++ ) {
    output_pids[pno] = fork();
    if ( output_pids[pno] == -1 ) {
      break;
    } else if ( output_pids[pno] == 0 ) {
      // 出力プロセスの処理
      read_data(10, semid, x);
      /* receiver(msgid, pno); */
      return pno;
    } else {
      printf("Process id of output child process is %d\n",output_pids[pno]);
    }      
  }

  if (pno) {
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
