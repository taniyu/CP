#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>

#define	SERV_UDP_PORT 20000

#define tvaltof(ts, tus) ((ts)+(double)(tus)/1000000)

#define	BLKSIZE	2048
unsigned char buf[BLKSIZE];

void err_msg(char *msg)
{
  perror(msg);
  exit(1);
}

int main(int argc, char *argv[])
{
  int port_no;
  int sockfd;
  struct sockaddr_in serv_addr, cli_addr;
  int cli_len = sizeof(cli_addr);
  int serv_len = sizeof(serv_addr);
  int seqnum,n;
  long sec,usec;
  struct timeval t;
  struct timezone z;

  port_no = (argc > 1) ? atoi(argv[1]) : SERV_UDP_PORT;
  printf("待ち受けのポート番号は %d です．\n"
	 "クライアントからのパケットを受信しヘッダ部分を表示すると同時に\n"
	 "id 部分に 0x20 と排他論理の演算を行いクライアントに返します．\n",
	 port_no);
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    err_msg("srever: can't open datagram socket");
  }

  bzero((char *) &serv_addr, serv_len);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(port_no);
 
  if (bind(sockfd, (struct sockaddr *)&serv_addr, serv_len) < 0) {
    err_msg("srever: can't bind local address");
  }
  
  for ( ; ; ) {
    n = recvfrom(sockfd, buf, BLKSIZE, MSG_WAITALL, (struct sockaddr *)&cli_addr, &cli_len);
    gettimeofday(&t, &z);
    if (n < 0) {
      err_msg("server: recvfrom error");
    }
    seqnum = (buf[ 1] << 24) | (buf[ 2] << 16) | (buf[ 3] << 8) | buf[ 4];
    sec = (buf[ 5] << 24) | (buf[ 6] << 16) | (buf[ 7] << 8) | buf[ 8];
    usec = (buf[ 9] << 24) | (buf[10] << 16) | (buf[11] << 8) | buf[12];
    printf("%c: %6ld at %9ld.%06ld (from %9ld.%06ld, 差は %f ミリ秒です)\n",
	   buf[0], seqnum, t.tv_sec, t.tv_usec, sec, usec,
	   (tvaltof(t.tv_sec, t.tv_usec) - tvaltof(sec, usec))*1000);
    fflush(stdout);
    buf[0] ^= 0x20;
    n = sendto(sockfd, buf, n, 0, (struct sockaddr *)&cli_addr, cli_len);
  }
}
