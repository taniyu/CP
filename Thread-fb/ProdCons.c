// 簡単な 生産者－消費者問題
//
// 生産者側はキーボード側から数字を入力し，それをバッファに入れる
// (入力終了あるいは負の数字が入力されると終わりとする)
// 消費者側はバッファから取り出す数字が正の場合は取り出しその秒数スリープする
// 負の場合は取り出さずに終了する
// バッファの管理変数へのアクセス時に排他制御を行なう
//

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUFSIZE 10

int buffer[BUFSIZE]; // バッファ
int readpoint;       // 読み出し位置
int writepoint;      // 書き込み位置
int num;             // バッファ中のデータ数

pthread_mutex_t lockval;

void producer()
{
  int val;
  char linebuf[20];

  do {
    if ((fgets(linebuf, 20, stdin) == NULL) || ((val = atoi(linebuf)) < 0)) val = -1;
    while (1) {
      pthread_mutex_lock(&lockval);
      if (num < BUFSIZE) break;   // バッファに空きが出れば待ちループを終わる
      pthread_mutex_unlock(&lockval);
    }
    buffer[writepoint] = val;  writepoint = (writepoint+1) % BUFSIZE;  num++;
    printf("Producer: enter %d to buffer (%d)\n", val, num);
    pthread_mutex_unlock(&lockval);
  } while (val >= 0);
  pthread_exit(NULL);
}

void consumer()
{
  int val;

  while (1) {
    while (1) {
      pthread_mutex_lock(&lockval);
      if (num) break;     // バッファにデータがあれば待ちループを終わる
      pthread_mutex_unlock(&lockval);
    }
    val = buffer[readpoint];
    if (val < 0) {        // 終了条件の検査
      pthread_mutex_unlock(&lockval);
      pthread_exit(NULL);
    }
    readpoint = (readpoint+1) % BUFSIZE;  num--;
    printf("Consumer: pick up %d from buffer (%d)\n", val, num);
    pthread_mutex_unlock(&lockval);
    if (val > 0) sleep(val);
  }
}

int main()
{
  pthread_t prod, cons;

  readpoint = writepoint = 0;  num = 0;
  pthread_mutex_init(&lockval, NULL);
  pthread_create(&prod, NULL, (void (*))producer, NULL);
  pthread_create(&cons, NULL, (void (*))consumer, NULL);
  pthread_join(prod, NULL);
  pthread_join(cons, NULL);
  pthread_exit(NULL);
}
