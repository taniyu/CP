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

  /* printf("行 %d\n", *tnum); */
  for (k1 = 0; k1 < SIZE; k1++) {
    z[*tnum][k1] = 0;
    for (k2 = 0; k2 < SIZE; k2++) {
      z[*tnum][k1] += x[*tnum][k2] * y[k2][k1];
    }
  }
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
  int thnum;
  time_t t;
  struct timeval start, end;

  srand((unsigned int)time(&t));
  init_arr(x);
  init_arr(y);

  gettimeofday(&start, NULL);
  // 行列
  for (thnum = 0; thnum < THNUM; thnum++) {
    inner_product(&thnum);
  }
  gettimeofday(&end, NULL);
  printf("%f\n", (double)tvaltof(end.tv_sec, end.tv_usec) - tvaltof(start.tv_sec, start.tv_usec));

  /* print_arr(z); */
  pthread_exit(NULL);
}
