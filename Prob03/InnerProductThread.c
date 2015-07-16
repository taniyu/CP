#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>

#define OLOOP 5
#define ILOOP 400000
#define SIZE 1000
#define THNUM 1000
#define tvaltof(ts, tus) ((ts)+(double)(tus)/1000000)

volatile double x[SIZE][SIZE];
volatile double y[SIZE][SIZE];
volatile double z[SIZE][SIZE];
pthread_mutex_t mutex;

void print_arr(volatile double arr[SIZE][SIZE]) {
  int k1, k2;
  for ( k1 = 0; k1 < SIZE; k1++ ) {
    for ( k2 = 0; k2 < SIZE; k2++ ) {
      printf("%4f", arr[k1][k2]);
    }
    puts("");
  }
}

void inner_product(int *tnum) {
  int k1,k2;
  int errcode;

  printf("Thread %d\n", *tnum);
  for (k1 = 0; k1 < SIZE; k1++) {
    z[*tnum][k1] = 0;
    for (k2 = 0; k2 < SIZE; k2++) {
      z[*tnum][k1] += x[*tnum][k2] * y[k2][k1];
    }
  }
  pthread_exit(tnum);  
}

void init_arr(volatile double arr[SIZE][SIZE]) {
  int k1, k2;
  for ( k1 = 0; k1 < SIZE; k1++ ) {
    for ( k2 = 0; k2 < SIZE; k2++ ) {
      arr[k1][k2] = rand() / (RAND_MAX + 1.0) * 10;
    }
  }
}

int main()
{
  pthread_t threads[THNUM];
  int params[THNUM];
  int *status;
  int thnum;
  time_t t;
  struct timeval start, end;

  srand((unsigned int)time(&t));
  init_arr(x);
  init_arr(y);

  pthread_mutex_init(&mutex, NULL);
  // 行列
  gettimeofday(&start, NULL);
  for (thnum = 0; thnum < THNUM; thnum++) {
    params[thnum] = thnum;
    pthread_create(&threads[thnum], NULL, (void (*))inner_product, &params[thnum]);
  }

  for (thnum = 0; thnum < THNUM; thnum++) {
    pthread_join(threads[thnum], (void **)&status);
    printf("Finish thread %d with return value %d\n", thnum, *status);
  }
  gettimeofday(&end, NULL);
  printf("%f\n", (double)tvaltof(end.tv_sec, end.tv_usec) - tvaltof(start.tv_sec, start.tv_usec));
  /* print_arr(z); */
  pthread_exit(NULL);
}
