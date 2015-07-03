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
#define SENDNUM 300

#define tvaltof(ts, tus) ((ts)+(double)(tus)/1000000)

unsigned char buf[BLKSIZE];
volatile int seqnum = 0;
char id;
int sockfd;
struct sockaddr_in cli_addr, serv_addr;
int cli_len = sizeof(cli_addr);
int serv_len = sizeof(serv_addr);

err_msg(char *msg)
{
  perror(msg);
  exit(1);
}

void catch_alarm()
{
  struct timeval t;
  struct timezone z;
  int i;
  
  buf[ 0] = id;
  buf[ 1] = (seqnum >> 24) & 0xff;
  buf[ 2] = (seqnum >> 16) & 0xff;
  buf[ 3] = (seqnum >> 8) & 0xff;
  buf[ 4] = seqnum & 0xff;
  gettimeofday(&t, &z);
  buf[ 5] = (t.tv_sec >> 24) & 0xff;
  buf[ 6] = (t.tv_sec >> 16) & 0xff;
  buf[ 7] = (t.tv_sec >> 8) & 0xff;
  buf[ 8] = t.tv_sec & 0xff;
  buf[ 9] = (t.tv_usec >> 24) & 0xff;
  buf[10] = (t.tv_usec >> 16) & 0xff;
  buf[11] = (t.tv_usec >> 8) & 0xff;
  buf[12] = t.tv_usec & 0xff;
  if (sendto(sockfd, buf, BLKSIZE, 0, (struct sockaddr *)&serv_addr, serv_len) < 0) {
    if (errno != EAGAIN) {
      err_msg("client: sendto error on socket");
      seqnum = SENDNUM; /* set terminate condition */
    } else {
      putchar('#');
    }
  } else {
    seqnum++;
  }
  signal(SIGALRM,catch_alarm);
}

int main(int argc, char *argv[])
{
  int port_no;
  int ipaddr;
  int seq, sec, usec, n;
  unsigned char buf2[BLKSIZE];
  struct timeval t;
  struct timezone z;
  struct sockaddr_in serv_addr2;
  int serv_len2;
  time_t et;
  struct hostent *retrieve;
  
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
  printf("%d.%d.%d.%d (%d) に %c で %f ミリ秒毎に %d バイトのデータを %d 回送ります．\n"
	 "同時にサーバからのパケットを受け取り，ヘッダ部分を表示します．\n",
         ipaddr & 0xff, (ipaddr >> 8) & 0xff, (ipaddr >> 16) & 0xff, (ipaddr >> 24) & 0xff,
         port_no, id, (double)INTERVAL/1000, BLKSIZE, SENDNUM);
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
  
  signal(SIGALRM,catch_alarm);
  intval.it_interval.tv_usec = intval.it_value.tv_usec = INTERVAL;
  intval.it_interval.tv_sec = intval.it_value.tv_sec = 0;
  setitimer(ITIMER_REAL, &intval, NULL);
  while (seqnum < SENDNUM) {
    serv_len2 = sizeof(serv_addr2);
    n = recvfrom(sockfd, buf2, BLKSIZE, MSG_WAITALL, (struct sockaddr *)&serv_addr2, &serv_len2);
    if (n > 0) {
      gettimeofday(&t, &z);
      seq = (buf2[ 1] << 24) | (buf2[ 2] << 16) | (buf2[ 3] << 8) | buf2[ 4];
      sec = (buf2[ 5] << 24) | (buf2[ 6] << 16) | (buf2[ 7] << 8) | buf2[ 8];
      usec = (buf2[ 9] << 24) | (buf2[10] << 16) | (buf2[11] << 8) | buf2[12];
      printf("%c: %6ld at %9ld.%06ld (from %9ld.%06ld, 差は %f ミリ秒です)\n",
	     buf2[0], seq, t.tv_sec, t.tv_usec, sec, usec,
	     (tvaltof(t.tv_sec, t.tv_usec) - tvaltof(sec, usec))*1000);
      fflush(stdout);
    }
  }
/* stop timer */
  intval.it_interval.tv_usec = intval.it_value.tv_usec = 0;
  intval.it_interval.tv_sec = intval.it_value.tv_sec = 0;
  setitimer(ITIMER_REAL, &intval, NULL);

  et = time(NULL);
  while (et+2 > time(NULL)) {
    n = recvfrom(sockfd, buf2, BLKSIZE, MSG_WAITALL, (struct sockaddr *)&serv_addr2, &serv_len2);
    if (n > 0) {
      gettimeofday(&t, &z);
      seq = (buf2[ 1] << 24) | (buf2[ 2] << 16) | (buf2[ 3] << 8) | buf2[ 4];
      sec = (buf2[ 5] << 24) | (buf2[ 6] << 16) | (buf2[ 7] << 8) | buf2[ 8];
      usec = (buf2[ 9] << 24) | (buf2[10] << 16) | (buf2[11] << 8) | buf2[12];
      printf("%c: %6ld at %9ld.%06ld (from %9ld.%06ld, 差は %f ミリ秒です)\n",
	     buf2[0], seq, t.tv_sec, t.tv_usec, sec, usec,
	     (tvaltof(t.tv_sec, t.tv_usec) - tvaltof(sec, usec))*1000);
      fflush(stdout);
    }
  }

  close(sockfd);
  return 0;
}
