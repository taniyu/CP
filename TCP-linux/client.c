#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
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
  int ipaddr;
  int sockfd;
  struct sockaddr_in serv_addr;
  int rflag, wflag, endflag;
  char sch, rch;
  struct hostent *retrieve;

  char s_buff[BUFF];
  char r_buff[BUFF], r_buff2[BUFF];
  char *tmp1;
  int rw_size = 1;
  struct timeval t1, t2;
  double r_sec, r_usec, diff;

  if (argc < 2) {
    fprintf(stderr,"Usage: client serv_addr [port_no]\n");
    exit(1);
  }
  port_no = (argc > 2) ? atoi(argv[2]) : SERV_TCP_PORT;
  if ((retrieve = gethostbyname(argv[1])) == NULL) {
    printf("Unknown host name: %s\n", argv[1]);
    exit(1);
  }
  ipaddr = *(unsigned int *)(retrieve->h_addr_list[0]);
  printf("%d.%d.%d.%d (%d) にアクセスします．Ctrl-A を押すと終了します．\n",
         ipaddr & 0xff, (ipaddr >> 8) & 0xff, (ipaddr >> 16) & 0xff, (ipaddr >> 24) & 0xff,
         port_no);
  bzero((char *)&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = *(unsigned int *)(retrieve->h_addr_list[0]);
  serv_addr.sin_port = htons(port_no);
  
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    err_msg("client: can't open datastream socket");
  }
  
  if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    err_msg("client: can't connect server address");
  }

  fcntl(fileno(stdin), F_SETFL, O_NONBLOCK);
  fcntl(fileno(stdout), F_SETFL, O_NONBLOCK);
  fcntl(sockfd, F_SETFL, O_NONBLOCK);
  rflag = 1; wflag = 0;
  while (1) {
    if (rflag == 1) {
      if (fgets(s_buff, BUFF, stdin)) {
        strtok(s_buff, "\n\0");
        if (s_buff[0] == '\001') {
          endflag = 0;
          break;	/* ^A */
        }
        rflag = 0;
      }
    }
    if (rflag == 0) {
      gettimeofday(&t1, NULL);
      sprintf(s_buff, "%s:%0.f.%0.f\n", s_buff, (double)t1.tv_sec, (double)t2.tv_usec);
      if (send(sockfd, s_buff, BUFF, 0) != -1) {
        rflag = 1;
      }
    }
    if (wflag == 0) {
      if (recv(sockfd, r_buff, BUFF, 0) != -1) {
        if (r_buff[0] == '\001') {
          endflag = 1;
          break;	/* ^A */
        }
        gettimeofday(&t2, NULL);
        tmp1 = r_buff;
        tmp1 = strchr(tmp1, ':'); tmp1++;
        tmp1 = strchr(tmp1, ':'); tmp1++;
        sscanf(tmp1, "%lf", &r_sec);
        wflag = 1;
      }
    }
    if (wflag == 1) {
      diff = (t1.tv_sec + t1.tv_usec / 1000000.0) + (t2.tv_sec + t2.tv_usec/ 1000000.0) - r_sec;
      strtok(r_buff, "\n\0");
      strtok(s_buff, "\n\0");
      sprintf(r_buff2, "%s\n%s\n%.0f.%.0f\n%f\n",s_buff, r_buff, (double)t2.tv_sec, (double)t2.tv_usec, diff);
      if (fprintf(stdout, "%s", r_buff2)) {
        wflag = 0;
      }
    }
  }
  if (endflag == 0) {
    send(sockfd, s_buff, BUFF, 0);
  }
  write(fileno(stdout), "\nEnd.\n", strlen("\nEnd.\n")+1);
  close(sockfd);
  return 0;
}
