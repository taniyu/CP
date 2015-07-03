#include <pthread.h>
#include <stdio.h>

#define OLOOP 5
#define ILOOP 400000

volatile int x;

void countup_print(int *tnum)
{
  int i, j;

  for (i = 0; i < OLOOP; i++) {
    printf("Thread %d, i = %d, x = %d\n", *tnum, i, x);
    for (j = 0; j < ILOOP; j++) x++;
  }
  pthread_exit(tnum);
}

int main()
{
  pthread_t threads[2];
  int param[2] = { 0, 1 };
  int *status;

  pthread_create(&threads[0], NULL, (void (*))countup_print, &param[0]);
  pthread_create(&threads[1], NULL, (void (*))countup_print, &param[1]);
  pthread_join(threads[0], (void **)&status);
  printf("Finish thread 0 with return value %d\n", *status);
  pthread_join(threads[1], (void **)&status);
  printf("Finish thread 1 with return value %d\n", *status);
  printf("Final x = %d\n", x);
  pthread_exit(NULL);
}
