#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>

#define MSGNUM  40
#define	MAXMSG	100
#define QSIZE	100

int main()
{
  int pid;
  int status;
  int i;
  int msgid;
  struct msgbuf {
    long mtype;
    char mtext[1];
  } *mbuf;
  int	msgsize;
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
  if ((mbuf = (struct msgbuf *)malloc(sizeof(struct msgbuf)+MAXMSG)) == NULL) {
    perror("memory");
    exit(1);
  }
  pid = fork();
  switch (pid) {
  case    0:
    printf("I am child process\n");
    for (i = 0; i < MSGNUM; i++) {
      if ((msgsize = msgrcv(msgid, mbuf, MAXMSG, 0, 0)) == -1) {
	perror("Message receive");
      } else {
	mbuf->mtext[msgsize] = '\0';
	printf("Child receives \"%s\"\n", mbuf->mtext);
      }
      usleep(100000);
    }
    return 0;
  case    -1:
    printf("Fork error\n");
    exit(1);
  default:
    printf("Process id of child process is %d\n",pid);
    mbuf->mtype = 100;
    for (i = 0; i < MSGNUM; i++) {
      snprintf(mbuf->mtext, MAXMSG, "%d番目のメッセージ!",i);
      msgsize = strlen(mbuf->mtext);
      if (msgsnd(msgid, mbuf, msgsize, 0)) {
	perror("Message Send");
      }
      printf("Parent sends \"%s\"\n", mbuf->mtext);
    }
    if (wait(&status) == -1) {
      perror("Wait error\n");
    } else {
      printf("Return value is %d\n",status);
    }
    if (msgctl(msgid, IPC_RMID, NULL)) {
      perror("shmctl");
    }
  }
}
