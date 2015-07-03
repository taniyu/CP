#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

#define	SERV_TCP_PORT 20000

void err_msg(char *msg)
{
  perror(msg);
  exit(1);
}

int main(int argc, char *argv[])
{
  int port_no;
  int sockid;
  int sockfd;
  struct sockaddr_in serv_addr, cli_addr;
  socklen_t cli_len = sizeof(cli_addr);
  char ch;
  char outbuf[10];
  int pid;
  int chnum = 0; /* number of child processes */
  int status;

  port_no = (argc > 1) ? atoi(argv[1]) : SERV_TCP_PORT;
  printf("待ち受けのポート番号は %d です．\n", port_no);
  printf("デーモンプログラムとして動作し，クライアントとの間にコネクションが作られると，\n"
	 "チャイルドプロセスを生成して処理を任せ，次のコネクション待ちになります．\n"
	 "チャイルドプロセスクライアントから送られた文字とその16進コードをクライアントに\n"
	 "返し，コンソール上には自分のプロセス番号とクライアントに返した文字列を表示しま\n"
	 "す．\n");
  if ((sockid = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    err_msg("srever: can't open datastream socket");
  }

  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family	  = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port	  = htons(port_no);
  
  if (bind(sockid, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    err_msg("srever: can't bind local address");
  }

  if (listen(sockid, 5) == -1) {
    err_msg("srever: listen failed");
  }
  
  while (1) {
    if (chnum) {
      /* for terminating child process */
      while (chnum && ((pid = waitpid(-1, &status, WNOHANG)) > 0)) {
	fprintf(stderr, "Terminate child process: %d\n", pid);
	chnum--;
      }
    }
    if ((sockfd = accept(sockid, (struct sockaddr *)&cli_addr, &cli_len)) < 0) {
      close(sockid);
      fprintf(stderr,"server: can't accept");
      break;
    }
    pid = fork();
    if (pid < 0) {              /* fork error */
      close(sockfd);
      close(sockid);
      break;
    } else if (pid > 0) {	/* parent process */
      close(sockfd);
      chnum++;
      continue;
    } 
    
    /* child process */
    pid = getpid();
    fprintf(stderr, "\nI am child process %d\n",pid);
    close(sockid);
    while (1) {
      if (read(sockfd, &ch, 1) == 1) {
	sprintf(outbuf,"%c [%02X] ", ch, ch & 0xff);
	write(sockfd, outbuf, strlen(outbuf));
	printf("%d: %s\n", pid, outbuf);
	if (ch == '\001') {
	  fprintf(stderr, "Finished %d\n", pid);
	  break;	/* ^A */
	}
      } else {
	fprintf(stderr, "Illegal termination\n");
	break;
      }
    }
    close(sockfd);
    return 0;
  }
  
  /* part of parent process */
  while (chnum) {
    /* terminating child process */
    if (waitpid(-1, &status, WNOHANG) > 0) chnum--;
  }
  return 0;
}
