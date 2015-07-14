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
#define QSIZE  10  // メッセージキュー領域の大きさ

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

// キューの初期化
void init_queue(_queue *x) {
  x->size = 0;
  x->wptr = 0;
  x->rptr = 0;
}

// データをキューに書き込む 入力側の処理
void write_data(int l, int semid, _queue *x, int pno)
{
  int count = 0;
  int data, sleep_time;
  int flag;
  while ( 1 ) {
    flag = 0;
    if ( count >= l ) { return; }
    data = rand() / (RAND_MAX + 1.0) * 61 + 20;
    P(semid);
    if ( x->size < QSIZE ) {
      x->buff[x->wptr++] = data;
      x->wptr %= QSIZE;
      x->size++;
      count++;
      flag = 1;
    }
    V(semid);
    if ( flag == 1) {
      printf("write process_id: %d size: %d writedata: %d", pno, x->size, data);
      sleep_time = rand() / (RAND_MAX + 1.0) * 61 + 20;
      printf(", sleep_time: %dm\n", sleep_time);
      usleep(sleep_time * 1000);
    }
  }
}

// データをキューから読み出す 出力側の処理
void read_data(int m, int semid, _queue *x, int pno)
{
  int count = 0;
  int data;
  int flag;
  while ( 1 ) {
    flag = 0;
    if ( count >= m ) { return; }
    P(semid);
    if ( x->size > 0 ) {
      data = x->buff[x->rptr++];
      x->rptr %= QSIZE;
      x->size--;
      count++;
      flag = 1;
    }
    V(semid);
    if ( flag == 1 ) {
      printf("read process_id: %d size: %d", pno, x->size);
      printf(", read_data: %d\n", data);
      usleep(data * 1000);
    }
  }
}

int main(int argc, char *argv[])
{
  int pid;
  int input_pids[MAX_PID];
  int output_pids[MAX_PID];
  int status;
  int shmid;
  _queue *x;
  int semid;
  struct sembuf sops;
  int output_pno;
  int input_pno;
  int k;
  time_t t;
  int output_num = 1;
  int orepeat_num = 10;
  int input_num = 1;
  int irepeat_num = 10;

  // 引数が指定されているとき
  if ( argc >= 3 ) {
    input_num = atoi(argv[1]);
    output_num = atoi(argv[2]);
    // 入力プロセス1 出力プロセス複数
    if ( input_num == 1 && output_num > 1 ) { irepeat_num *= output_num; }
    // 入力プロセス複数 出力プロセス1
    if ( input_num > 1 && output_num == 1 ) { orepeat_num *= input_num; }
  }
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
  init_queue(x);

  semid = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
  if (semid == -1) {
    perror("semget");
    exit(1);
  }
  if (semctl(semid, 0, SETVAL, 1) == -1) {
    perror("semctl at initializing value of semaphore"); 
  }

  // 入力用プロセス生成
  for ( input_pno = 0; input_pno < input_num; input_pno++ ) {
    input_pids[input_pno] = fork();
    if ( input_pids[input_pno] == -1 ) {
      break;
    } else if (input_pids[input_pno] == 0) {
      // 入力プロセスの処理
      write_data(irepeat_num, semid, x, input_pno);
      return input_pno;
    } else {
      printf("Process id of input child process is %d\n", input_pids[input_pno]);
    }      
  }

  // 出力用プロセス生成
  for ( output_pno = 0; output_pno < output_num; output_pno++ ) {
    output_pids[output_pno] = fork();
    if ( output_pids[output_pno] == -1 ) {
      break;
    } else if ( output_pids[output_pno] == 0 ) {
      // 出力プロセスの処理
      read_data(orepeat_num, semid, x, output_pno);
      return output_pno;
    } else {
      printf("Process id of output child process is %d\n", output_pids[output_pno]);
    }      
  }

  if ((input_pno + output_pno)) {
    for ( k = 0; k < input_pno + output_pno; k++ ) {
      if (wait(&status) == -1) { perror("Wait error\n"); } 
      else { printf("Return value is %d\n",status); }
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
