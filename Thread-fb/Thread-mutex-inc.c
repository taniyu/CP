#include <pthread.h>
#include <stdio.h>
#include <errno.h>

#ifndef THNUM
#define THNUM 2
#endif

#define OLOOP 5
#define ILOOP 400000

volatile int x;
pthread_mutex_t mutex;

void countup_print(int *tnum)
{
  int i,j;
  int errcode;

  for (i = 0; i < OLOOP; i++) {
    printf("Thread %d, i = %d, x = %d\n", *tnum, i, x);
    for (j = 0; j < ILOOP; j++) {
      pthread_mutex_lock(&mutex);
      x++;
      pthread_mutex_unlock(&mutex);
    }
  }
  pthread_exit(tnum);
}

int main()
{
  pthread_t threads[THNUM];
  int params[THNUM];
  int *status;
  int thnum;

    pthread_mutex_init(&mutex, NULL);
    for (thnum = 0; thnum < THNUM; thnum++) {
      params[thnum] = thnum;
      pthread_create(&threads[thnum], NULL, (void (*))countup_print, &params[thnum]);
    }
    for (thnum = 0; thnum < THNUM; thnum++) {
      pthread_join(threads[thnum], (void **)&status);
      printf("Finish thread %d with return value %d\n", thnum, *status);
    }
    printf("Final x = %d\n", x);
    pthread_exit(NULL);
}
