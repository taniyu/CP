// ロックのテスト3
//
// クリティカルリージョンに入ることを trylock でチェックすることにより
// ブロックさせない
//

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define LOOPNUM 10
#define THNUM 5

pthread_mutex_t lockval;

void enter(int *thnum)
{
  int loop;

  for (loop = 0; loop < LOOPNUM; loop++) {
    while (pthread_mutex_trylock(&lockval)) {  // 1秒おきにロックの検査
      printf("%d (loop = %d): Wait for lock\n", *thnum, loop);
      sleep(1);
    }
    /* critical region start */
    printf("%d (loop = %d): Enter Critical Region\n", *thnum, loop);
    sleep(rand() % 5);
    printf("%d (loop = %d): Exit Critical Region\n", *thnum, loop);
    /* critical region end */
    pthread_mutex_unlock(&lockval);
    sleep(rand() % 5);
  }
  pthread_exit(NULL);
}

int main()
{
  int i;
  pthread_t th[THNUM];
  int params[THNUM];

  pthread_mutex_init(&lockval, NULL);
  for(i = 0 ; i < THNUM ; i++) {
    params[i] = i;
    pthread_create(&th[i], NULL, (void (*))enter, &params[i]);
  }
  for(i = 0 ; i < THNUM ; i++) {
    pthread_join(th[i], NULL);
  }
  pthread_exit(NULL);
}
