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
char buf[BLKSIZE];

void err_msg(char *msg)
{
  perror(msg);
  exit(1);
}

/* ファイルに指定されたログデータを書き込む */
int output_log(char *addr, char *str)
{
  FILE *fp;
  /* ファイルオープン失敗 */
  if ( (fp = fopen("access.log", "a")) == NULL ) {
    return -1;
  }
  fprintf(fp, "%s %s\n", addr, str);
  fclose(fp);
  return 1;
}

/* :以降の文字をaに置換する */
void replace_str(char *str)
{
  int flag = 0;
  while ( *str != '\0' ) {
    if ( flag == 1 ) { *str = '\0'; }
    if ( *str == ':' ) {
      flag = 1;
      str++;
      *str = 'a';
    }
    str++;
  }
}

int main(int argc, char *argv[])
{
  int port_no;
  int sockfd;
  struct sockaddr_in serv_addr, cli_addr;
  int cli_len = sizeof(cli_addr);
  int serv_len = sizeof(serv_addr);
  int n;
  struct timeval ts;
  char send_buff[BLKSIZE];

  port_no = (argc > 1) ? atoi(argv[1]) : SERV_UDP_PORT;
  printf("待ち受けのポート番号は %d です．\n", port_no);
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
  
  while ( 1 ) {
    n = recvfrom(sockfd, buf, BLKSIZE, MSG_WAITALL, (struct sockaddr *)&cli_addr, &cli_len);
    if ( n > 0) {
      // ファイルに書き込む
      output_log(inet_ntoa(cli_addr.sin_addr), buf);
      gettimeofday(&ts, NULL);
      // :以降の文字を置換
      replace_str(buf);
      sprintf(send_buff, "%s:%0.f.%0.f", buf, (double)ts.tv_sec, (double)ts.tv_usec);
      n = sendto(sockfd, send_buff, strlen(send_buff)+1, 0, (struct sockaddr *)&cli_addr, cli_len);        
      if ( n < 0 ) {
        err_msg("server: sendto error");
        break;
      }      
    } else if ( n == -1 ) {
      err_msg("server: recvfrom error");
      break;
    }
  }
    /* fflush(stdout); */
  printf("end\n");
  return 0;
}
