#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <sys/time.h>

#define	SERV_TCP_PORT 20000
#define BUFF 256

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
  int cli_len = sizeof(cli_addr);
  int rflag, wflag, endflag, iobytes;
  char sch, rch;

  char s_buff[BUFF], r_buff[BUFF], tmp1[BUFF];
  struct timeval ts;
  int k;

  port_no = (argc > 1) ? atoi(argv[1]) : SERV_TCP_PORT;
  printf("待ち受けのポート番号は %d です．Ctrl-A を押すと終了します．\n", port_no);
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
  
  if ((sockfd = accept(sockid, (struct sockaddr *)&cli_addr, &cli_len)) < 0) {
    err_msg("server: can't accept");
  }
  
  fcntl(fileno(stdin), F_SETFL, O_NONBLOCK);
  fcntl(fileno(stdout), F_SETFL, O_NONBLOCK);
  fcntl(sockfd, F_SETFL, O_NONBLOCK);
  rflag = 1; wflag = 0;
  while (1) {
    if (rflag == 0) {
      if ((iobytes = send(sockfd, s_buff, BUFF, 0)) != -1) {
        rflag = 1;
      } else if (iobytes == -1) {
        if (errno != EAGAIN) {
          perror("write");
          break;
        }
      }
      wflag = 0;
    }
    if (wflag == 0) {
      if ((iobytes = recv(sockfd, r_buff, BUFF, 0)) != -1) {
        gettimeofday(&ts, NULL);
        sscanf(r_buff, "%[^':']", tmp1);
        sprintf(s_buff, "%s:a:%.0f.%.0f", tmp1, (double)ts.tv_sec, (double)ts.tv_usec);
        if (r_buff[0] == '\001') {
          endflag = 1;
          break;	/* ^A */
        }
        wflag = 1;
        rflag = 0;
      } else if (iobytes == -1) {
        if (errno != EAGAIN) {
          perror("read");
          break;
        }
      }
    }
  }
  if (endflag == 0) {
    send(sockfd, s_buff, BUFF, 0);
  }
  close(sockfd);
  close(sockid);
  return 0;
}
