// 各スレッドが独立して表示，終了時刻を返値で返す
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#define THNUM 10
#define LOOP  1000

void test(int *thnum)
{
  int loop;
  struct timeval *tv = malloc(sizeof(struct timeval));

  for (loop = 0; loop < LOOP; loop++) {
    printf("スレッド %d は %d を表示しました\n", *thnum, loop);
  }
  gettimeofday(tv, NULL);
  pthread_exit(tv);
}

int main()
{
  pthread_t threads[THNUM];
  int params[THNUM];
  int thnum;
  struct timeval now;
  struct timeval *status;

  gettimeofday(&now, NULL);
  for (thnum = 0; thnum < THNUM; thnum++) {
    params[thnum] = thnum;
    pthread_create(&threads[thnum], NULL, (void(*))test, &params[thnum]);
  }
  for (thnum = 0; thnum < THNUM; thnum++) {
    pthread_join(threads[thnum], (void **)&status);
    fprintf(stderr, "スレッド %d は開始から %.3f ミリ秒後に終了しました\n", thnum,
            ((double)(status->tv_sec*1000000+status->tv_usec) -
             ((double)(now.tv_sec)*1000000+now.tv_usec))/1000);
  }
  pthread_exit(NULL);
}
