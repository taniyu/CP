#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

#define SERV_UDP_PORT 20000
#define INTERVAL 200000 /* in micro second */
struct itimerval intval;

#define BLKSIZE 1280
#define SENDNUM 10
#define BUFF 256

#define tvaltof(ts, tus) ((ts)+(double)(tus)/1000000)

volatile int seqnum = 0;
char id;
int sockfd;
struct sockaddr_in cli_addr, serv_addr;
int cli_len = sizeof(cli_addr);
int serv_len = sizeof(serv_addr);

void err_msg(char *msg)
{
  perror(msg);
  exit(1);
}

/* 文字列を数値があるところまで動く */
char *search_num(char *str)
{
  while( *str ) {
    if ( '0' <= *str && *str <= '9' ) { return str; }
    str++;
  }
  return NULL;
}

int main(int argc, char *argv[])
{
  int port_no;
  int ipaddr;
  int seq, sec, usec, n;
  /* unsigned char buf[BLKSIZE]; */
  char buf[BLKSIZE];
  char *tmp1;
  /* unsigned char buf2[BLKSIZE]; */
  char buf2[BLKSIZE], output_buff[BLKSIZE];
  struct timeval t, t1, t2;
  struct timezone z;
  struct sockaddr_in serv_addr2;
  int serv_len2;
  time_t et;
  struct hostent *retrieve;
  double r_sec, diff;
  int rflag, wflag, endflag;

  if (argc < 3) {
    fprintf(stderr,"Usage: client serv_addr id(character) [port_no]\n");
    exit(1);
  }
  port_no = (argc > 3) ? atoi(argv[2]) : SERV_UDP_PORT;
  if ((retrieve = gethostbyname(argv[1])) == NULL) {
    printf("Unknown host name: %s\n", argv[1]);
    exit(1);
  }
  id = *argv[2];
  ipaddr = *(unsigned int *)(retrieve->h_addr_list[0]);
  printf("%d.%d.%d.%d (%d) に 入力した文字列を送信します\n",
         ipaddr & 0xff, (ipaddr >> 8) & 0xff, (ipaddr >> 16) & 0xff, (ipaddr >> 24) & 0xff,
         port_no);
  bzero((char *)&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = *(unsigned int *)(retrieve->h_addr_list[0]);
  serv_addr.sin_port = htons(port_no);
  
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    err_msg("client: can't open datagram socket");
  }
  
  bzero((char *)&cli_addr, cli_len);
  cli_addr.sin_family = AF_INET;
  cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  cli_addr.sin_port = htons(0);
  if (bind(sockfd, (struct sockaddr *)&cli_addr, cli_len) < 0) {
    err_msg("client: can't bind local address");
  }
  fcntl(sockfd, F_SETFL, O_NONBLOCK);
  
  rflag = 1; wflag = 0;
  while ( 1 ) {
    if ( rflag == 1 ) { 
      if ( fgets(buf, BLKSIZE, stdin) ) { 
        strtok(buf, "\n\0");
        if ( buf[0] == '\001' ) {
          endflag = 0;
          break;
        }
        rflag = 0;
      }
    }
    if ( rflag == 0 ) { 
      gettimeofday(&t1, NULL);
      sprintf(buf, "%s:%0.f.%0.f", buf, (double)t1.tv_sec, (double)t1.tv_usec);
      if ( sendto(sockfd, buf, BLKSIZE, 0, (struct sockaddr *)&serv_addr, serv_len) > 0) {
        rflag = 1;
      }
    }

    while ( wflag == 0 ) {
      n = recvfrom(sockfd, buf2, BLKSIZE, MSG_WAITALL, (struct sockaddr *)&serv_addr2, &serv_len2);
      gettimeofday(&t2, NULL);  /* 受け取った時間を取得 */
      if ( n > 0 ) { 
        if( buf2[0] == '\001' ) {
          endflag = 1;
          break;
        }
        r_sec = atof(search_num(buf2)); /* 時間の部分を取得 */
        wflag = 1;
      }
    }

    if ( wflag == 1 ) {
      diff = tvaltof(t1.tv_sec, t1.tv_usec) + tvaltof(t2.tv_sec, t2.tv_usec) - r_sec;
      sprintf(output_buff, "%s\n%s\n%.0f.%.0f\n%f", buf, buf2, (double)t2.tv_sec, (double)t2.tv_usec, diff);
      if ( fprintf(stdout, "%s\n", output_buff) ) { wflag = 0; }
    }
  }

  if ( endflag == 0 ) {
    fprintf(stdout, "%d.%d.%d.%d (%d)との通信終了\n", ipaddr & 0xff, (ipaddr >> 8) & 0xff, (ipaddr >> 16) & 0xff, (ipaddr >> 24) & 0xff, port_no);
  }

  close(sockfd);
  return 0;
}
