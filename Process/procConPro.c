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

#define MAX_PID 100  // プロセスの最大数
#define QSIZE  10  // メッセージキュー領域の大きさ
#define ROOP 2  // データの数

// キューの宣言
typedef struct {
  int wptr;  // 入力位置
  int rptr;   // 出力位置
  int buff[QSIZE]; // データ格納用配列
  int size;   // データ数
} Queue;


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
void init_queue(Queue *msg_queue) {
  msg_queue->size = 0;
  msg_queue->wptr = 0;
  msg_queue->rptr = 0;
}

// データをキューに書き込む 入力側の処理
void write_data(int l, int semid, Queue *msg_queue, int pno)
{
  int count = 0;
  int data, sleep_time;

  srand((unsigned int)time(NULL) + pno);

  while ( 1 ) {
    if ( count >= l ) { return; }
    data = rand() / (RAND_MAX + 1.0) * 61 + 20;
    P(semid);
    if ( msg_queue->size < QSIZE ) {
      msg_queue->buff[msg_queue->wptr++] = data;
      msg_queue->wptr %= QSIZE;
      msg_queue->size++;
      count++;
      printf("write process_id: %d buf_size: %d writedata: %d\n", pno, msg_queue->size, data);
      sleep_time = rand() / (RAND_MAX + 1.0) * 61 + 20;
      usleep(sleep_time * 1000);
    }
    V(semid);
  }
}

// データをキューから読み出す 出力側の処理
void read_data(int m, int semid, Queue *msg_queue, int pno)
{
  int count = 0;
  int data;
  while ( 1 ) {
    if ( count >= m ) { return; }
    P(semid);
    if ( msg_queue->size > 0 ) {
      data = msg_queue->buff[msg_queue->rptr++];
      msg_queue->rptr %= QSIZE;
      msg_queue->size--;
      count++;
      printf("read  process_id: %d buf_size: %d", pno, msg_queue->size);
      printf(" readdata:  %d\n", data);
      usleep(data * 1000);
    }
    V(semid);
  }
}

int main(int argc, char *argv[])
{
  int pids[MAX_PID];
  int status;
  int shmid;
  Queue *msg_queue;
  int semid;
  int i = 0, k;
  int output_num = 1;
  int orepeat_num = ROOP;
  int input_num = 1;
  int irepeat_num = ROOP;
  int process_flag = 0;
  int input_id = 0;
  int output_id = 0;

  // 引数が指定されているとき
  if ( argc >= 3 ) {
    input_num = atoi(argv[1]);
    output_num = atoi(argv[2]);
    irepeat_num = output_num * ROOP;
    orepeat_num = input_num * ROOP;
  }

  shmid = shmget(IPC_PRIVATE, sizeof(Queue), IPC_CREAT | 0666);
  if (shmid == -1) {
    perror("shmget");
    exit(1);
  }
  msg_queue = (Queue *)shmat(shmid, NULL, 0);
  if (msg_queue == (Queue *)-1) {
    perror("shmat");
    exit(1);
  }

  // キューの初期化
  init_queue(msg_queue);

  semid = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
  if (semid == -1) {
    perror("semget");
    exit(1);
  }
  if (semctl(semid, 0, SETVAL, 1) == -1) {
    perror("semctl at initializing value of semaphore"); 
  }

  // プロセスの生成
  while( (input_num + output_num) > 0 ) {
    pids[i] = fork();
    // 作成するプロセスを決定
    if ( input_num > 0 && i%2 == 0 ) { process_flag = 0; }  // 入力プロセス生成
    else if ( output_num > 0 && i%2 == 1 ) { process_flag = 1; }  // 出力プロセス生成
    else if ( 1 > input_num ) { process_flag = 1; }  // 出力プロセス生成
    else if ( 1 > output_num ) { process_flag = 0; } // 入力プロセス生成
    // 生成できたプロセスに応じて処理を分ける
    if ( pids[i] == -1 ) {
      break;
    } else if (pids[i] == 0) {
      // process_flagに応じて生成するプロセスを変更
      if ( process_flag == 0 ) { write_data(irepeat_num, semid, msg_queue, input_id); }
      else { read_data(orepeat_num, semid, msg_queue, output_id); }
      return i;
    } else {
      // 親の処理
      if ( process_flag == 0 ) {
        /* printf("Process id of write child process is %d\n", pids[i]); */
        input_id++;
        input_num--;
      } else {
        /* printf("Process id of read child process is %d\n", pids[i]); */
        output_id++;
        output_num--;
      }
    }      
    i++;
  }

  // 親の処理
  if ( i ) {
    for ( k = 0; k < i; k++ ) {
      if (wait(&status) == -1) { perror("Wait error\n"); } 
      /* else { printf("Return value is %d\n",status); } */
    }
    if (shmctl(shmid, IPC_RMID, NULL)) { perror("shmctl"); }
    if (semctl(semid, 0, IPC_RMID)) { perror("semctl"); }    
  }
  return 0;
}
