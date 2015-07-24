#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <time.h>

#define CHNUM    3  // 子プロセスの個数
#define MSGNUM   40
#define	MAXMSG  100
#define QSIZE   100  // メッセージキュー領域の大きさ
#define ENDMSG "end"
#define MAXCHNUM 3
#define BUFF 256

typedef enum { STONE, SCISSORS, PAPER } Hand;
typedef enum { FALSE, TRUE } Bool;

typedef struct {
  long mtype;
  char mtext[MAXMSG+1];
} Msgbuf;

// じゃんけんを行う子プロセス CPU
void cpu_player(int msgid, int chno, int seed)
{
  Hand hand;
  Msgbuf mbuf;
  int msgsize;

  srand((unsigned int)time(NULL) + seed);
  rand(); rand(); rand(); rand(); rand();
  hand = (int)(rand() / (RAND_MAX + 1.0) * 3);
  printf("chno:%d hand:%d seed:%d\n", chno, hand, seed);

  do {
    if ((msgsize = msgrcv(msgid, &mbuf, MAXMSG, 0, 0)) == -1) {
      perror("Message receive");
    } else {
      mbuf.mtext[msgsize] = '\0';
      printf("Child %d receives \"%s\"\n", chno, mbuf.mtext);
    }
    usleep(100000);
  } while (strcmp(mbuf.mtext, ENDMSG) != 0);

}

// メッセージ送信側 (親プロセス)
// 指定されたメッセージを送る
void sender(int msgid, int chno, char *msg)
{
  int i;
  Msgbuf mbuf;
  int msgsize;

  mbuf.mtype = 100;
  for (i = 0; i < MSGNUM; i++) {
    snprintf(mbuf.mtext, MAXMSG, "%d番目のメッセージ!",i);
    msgsize = strlen(mbuf.mtext);
    if (msgsnd(msgid, &mbuf, msgsize, 0)) {
      perror("Message Send");
    }
    printf("Parent sends \"%s\"\n", mbuf.mtext);
  }
  for (i = 0; i < chno; i++) {
    strncpy(mbuf.mtext, ENDMSG, MAXMSG);
    msgsize = strlen(mbuf.mtext);
    if (msgsnd(msgid, &mbuf, msgsize, 0)) {
      perror("Message Send");
    }
    printf("Parent sends \"%s\"\n", mbuf.mtext);
  }
}

// メッセージ受信側 (子プロセス)
void receiver(int msgid, int chno)
{
  int i;
  Msgbuf mbuf;
  int msgsize;

  printf("I am child process (No.%d)\n", chno);
  do {
    if ((msgsize = msgrcv(msgid, &mbuf, MAXMSG, 0, 0)) == -1) {
      perror("Message receive");
    } else {
      mbuf.mtext[msgsize] = '\0';
      printf("Child %d receives \"%s\"\n", chno, mbuf.mtext);
    }
    usleep(100000);
  } while (strcmp(mbuf.mtext, ENDMSG) != 0);
}

// 手の入力
Bool input_hand(Hand *hand)
{
  char buff[BUFF];
  int i;
  puts("手を入力してください．");
  while ( 1 ) {
    if ( fgets(buff, BUFF, stdin) == NULL ) {
      return FALSE;
    }
    for ( i = 0; buff[i] != '\0'; i++ ) {
      // 入力が正しいとき
      if ( '0' <= buff[i] && buff[i] <= '2' ) {
        *hand = buff[i] - '0';
        return TRUE;
      }
      // ゲームの終了
      if ( buff[i] == 'q' ) {
        return FALSE;
      }
      puts("入力が正しくありません，0~2までの数値を入力してください．");
    }
    
  }
  
  return TRUE;
}

int main()
{
  int pid[MAXCHNUM];
  int pno;
  int status;
  int i;
  int msgid;
  struct msqid_ds qds;
  int seeds[MAXCHNUM];
  Hand hand;
  Bool flag;
  Msgbuf mbuf;

  msgid = msgget(IPC_PRIVATE, IPC_CREAT | 0666);
  if (msgid == -1) {
    perror("msgget");
    exit(1);
  }
  if (msgctl(msgid, IPC_STAT, &qds)) {
    perror("msgctl");
    exit(1);
  }
  printf("Change queue size %lu to %d\n",qds.msg_qbytes, QSIZE);
  qds.msg_qbytes = QSIZE;
  if (msgctl(msgid, IPC_SET, &qds)) {
    perror("msgctl");
    exit(1);
  }

  // 乱数シード
  srand((unsigned int)time(NULL));
  rand(); rand(); rand(); rand(); rand();

  for (pno = 0; pno < MAXCHNUM; pno++) {
    // 100未満の乱数を生成
    seeds[pno] = rand() / (RAND_MAX + 1.0) * 100;
    pid[pno] = fork();
    if (pid[pno] == -1) {
      printf("Fork error\n");
      break;
    } else if (pid[pno] == 0) {
      cpu_player(msgid, pno, seeds[pno]);
      /* receiver(msgid, pno); */
      return pno;
    } else {
      printf("Process id of child process is %d\n",pid[pno]);
    }
  }

  // 親プロセスの処理
  if (pno) {	// 子プロセスを1つ以上持っている
    while ( 1 ) {
       flag = input_hand(&hand);
       printf("%d\n", hand);
       // flag が FASLEであればゲームを強制終了
       if ( flag == FALSE ) {
         // 子に終了用のメッセージを送る
         for ( i = 0; i < MAXCHNUM; i++ ) {
           mbuf.mtype = 100;
           strncpy(mbuf.mtext, ENDMSG, MAXMSG);
           if (msgsnd(msgid, &mbuf, strlen(mbuf.mtext), 0)) {
             perror("Message Send");
           }
           printf("Parent sends \"%s\"\n", mbuf.mtext);
         }
         break;
       }
    }
    /* sender(msgid, pno); */
    for (i = 0; i < pno; i++) {
      if (wait(&status) == -1) {
        perror("Wait error\n");
      } else {
        printf("Terminates Child #%d\n", (status >> 8) & 0xff);
      }
    }
  }
  if (msgctl(msgid, IPC_RMID, NULL)) {
    perror("shmctl");
  }
}
