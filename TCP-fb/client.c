#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
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
  int ipaddr;
  int sockfd;
  struct sockaddr_in serv_addr;
  int rflag, wflag, endflag;
  char sch, rch;
  struct hostent *retrieve;
  
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
      if (read(fileno(stdin), &sch, 1) == 1) {
	if (sch == '\001') {
	  endflag = 0;
	  break;	/* ^A */
	}
	rflag = 0;
      }
    }
    if (rflag == 0) {
      if (write(sockfd, &sch, 1) == 1) {
	rflag = 1;
      }
    }
    if (wflag == 0) {
      if (read(sockfd, &rch, 1) == 1) {
	if (rch == '\001') {
	  endflag = 1;
	  break;	/* ^A */
	}
	wflag = 1;
      }
    }
    if (wflag == 1) {
      if (write(fileno(stdout), &rch, 1) == 1) {
	wflag = 0;
      }
    }
  }
  if (endflag == 0) {
    while (write(sockfd, &sch, 1) != 1);
  }
  write(fileno(stdout), "\nEnd.\n", strlen("\nEnd.\n")+1);
  close(sockfd);
  return 0;
}
