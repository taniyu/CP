#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>

#define CHNUM    5  // 子プロセスの個数
#define MSGNUM   40
#define	MAXMSG  100
#define QSIZE   100  // メッセージキュー領域の大きさ
#define ENDMSG "end"


// メッセージ送信側 (親プロセス)
void sender(int msgid, int chno)
{
  int i;
  struct msgbuf {
    long mtype;
    char mtext[MAXMSG+1];
  } mbuf;
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
  struct msgbuf {
    long mtype;
    char mtext[MAXMSG+1];
  } mbuf;
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

int main()
{
  int pid[CHNUM];
  int pno;
  int status;
  int i;
  int msgid;
  struct msqid_ds qds;
  
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
  for (pno = 0; pno < CHNUM; pno++) {
    pid[pno] = fork();
    if (pid[pno] == -1) {
      printf("Fork error\n");
      break;
    } else if (pid[pno] == 0) {
      receiver(msgid, pno);
      return pno;
    } else {
      printf("Process id of child process is %d\n",pid[pno]);
    }
  }

  // 親プロセスの処理
  if (pno) {	// 子プロセスを1つ以上持っている
    sender(msgid, pno);
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
