#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef CONS_NUM
#define CONS_NUM 5
#endif

#define BUFSIZE 10

int buffer[BUFSIZE];
int readpoint;
int writepoint;
int num;

pthread_mutex_t lockval;
pthread_cond_t  notempty;   // バッファが空からデータが入った状態になった
pthread_cond_t  notfull;    // バッファが満杯から空きが出た状態になった

void producer()
{
  int val;
  char linebuf[20];

  do {
    if ((fgets(linebuf, 20, stdin) == NULL) || ((val = atoi(linebuf)) < 0)) val = -1;
    pthread_mutex_lock(&lockval);
    while (num >=  BUFSIZE) {
      pthread_cond_wait(&notfull, &lockval);  // データを消費したシグナルを待つ
    }
    buffer[writepoint] = val;  writepoint = (writepoint+1) % BUFSIZE;  num++;
    printf("Producer: enter %d to buffer (%d)\n", val, num);
    pthread_cond_signal(&notempty);           // データが入力したシグナルを送る
    pthread_mutex_unlock(&lockval);
  } while (val >= 0);
  // 全部の消費者にシグナルを送る．これがなければ，1つのコンディション待ちしか終わらない
  pthread_cond_broadcast(&notempty);
  pthread_exit(NULL);
}

void consumer(int *consNo)  // 消費者にIDを付けた
{
  int val;

  while(1) {
    pthread_mutex_lock(&lockval);
    while (num == 0) {
      pthread_cond_wait(&notempty, &lockval);  // データを入力したシグナルを待つ
    }
    if (buffer[readpoint] < 0) {
      pthread_mutex_unlock(&lockval);
      pthread_exit(NULL);
    }
    val = buffer[readpoint];  readpoint = (readpoint+1) % BUFSIZE;  num--;
    printf("Consumer %d: pick up %d from buffer (%d)\n", *consNo, val, num); // ID表示
    pthread_cond_signal(&notfull);             // データを消費したシグナルを送る
    pthread_mutex_unlock(&lockval);
    if (val > 0) sleep(val);
  }
}

int main()
{
  pthread_t prod, cons[CONS_NUM];
  int params[CONS_NUM];
  int thnum;

  readpoint = writepoint = 0;  num = 0;
  pthread_mutex_init(&lockval, NULL);
  pthread_cond_init(&notempty, NULL);
  pthread_cond_init(&notfull, NULL);
  pthread_create(&prod, NULL, (void (*))producer, NULL);
  for (thnum = 0; thnum < CONS_NUM; thnum++) {
    params[thnum] = thnum;
    pthread_create(&cons[thnum], NULL, (void (*))consumer, &params[thnum]);
  }
  pthread_join(prod, NULL);
  for (thnum = 0; thnum < CONS_NUM; thnum++) {
    pthread_join(cons[thnum], NULL);
  }
  pthread_exit(NULL);
}
