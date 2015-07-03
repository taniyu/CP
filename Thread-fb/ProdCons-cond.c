// 簡単な 生産者－消費者問題
//
// バッファの管理変数へのアクセス時に排他制御を行なう
// 相手の処理待ちになる場合はコンディションを用いる
//

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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
  pthread_exit(NULL);
}

void consumer()
{
  int val;

  while(1) {
    pthread_mutex_lock(&lockval);
    while (num == 0) {
      pthread_cond_wait(&notempty, &lockval);  // データを入力したシグナルを待つ
    }
    val = buffer[readpoint];
    if (val < 0) {
      pthread_mutex_unlock(&lockval);
      pthread_exit(NULL);
    }
    readpoint = (readpoint+1) % BUFSIZE;  num--;
    printf("Consumer: pick up %d from buffer (%d)\n", val, num);
    pthread_cond_signal(&notfull);             // データを消費したシグナルを送る
    pthread_mutex_unlock(&lockval);
    if (val > 0) sleep(val);
  }
}

int main()
{
  pthread_t prod, cons;

  readpoint = writepoint = 0;  num = 0;
  pthread_mutex_init(&lockval, NULL);
  pthread_cond_init(&notempty, NULL);
  pthread_cond_init(&notfull, NULL);
  pthread_create(&prod, NULL, (void (*))producer, NULL);
  pthread_create(&cons, NULL, (void (*))consumer, NULL);
  pthread_join(prod, NULL);
  pthread_join(cons, NULL);
  pthread_exit(NULL);
}
