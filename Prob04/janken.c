#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <time.h>

#define CHNUM    3  // 子プロセスの個数
#define MSGNUM   40
#define	MAXMSG  100
#define QSIZE   100  // メッセージキュー領域の大きさ
#define ENDMSG "end"
#define DECMSG "decide"
#define MAXCHNUM 10
#define BUFF 256
#define KIND 3

typedef enum { STONE, SCISSORS, PAPER } Hand;
typedef enum { LOSE, WIN, DRAW } Result;
typedef enum { FALSE, TRUE } Bool;

typedef struct {
  long mtype;
  char mtext[MAXMSG+1];
} Msgbuf;

char *g_hand_str[KIND] = { "グー", "チョキ", "パー" };
char *g_result_str[KIND] = { "負け", "勝ち", "引き分け" };

// じゃんけんを行う子プロセス CPU
void cpu_player(int recv_msgid, int send_msgid, int chno, int seed)
{
  Hand hand;
  Msgbuf mbuf;
  int msgsize;

  srand((unsigned int)time(NULL) + seed);
  rand(); rand(); rand(); rand(); rand();

  do {
    if ((msgsize = msgrcv(recv_msgid, &mbuf, MAXMSG, 0, 0)) == -1) {
      perror("Message receive");
    } else {
      mbuf.mtext[msgsize] = '\0';
      /* printf("Child %d receives \"%s\"\n", chno, mbuf.mtext); */
    }
    if ( strcmp(mbuf.mtext, DECMSG) == 0 ) {
      hand = (int)(rand() / (RAND_MAX + 1.0) * KIND);
      /* printf("chno:%d hand:%d seed:%d\n", chno, hand, seed); */
      snprintf(mbuf.mtext, MAXMSG, "%d:%d", chno, hand);
      msgsize = strlen(mbuf.mtext);
      if (msgsnd(send_msgid, &mbuf, msgsize, 0)) {
        perror("Message Send");
      }
    }
    usleep(100000);
  } while (strcmp(mbuf.mtext, ENDMSG) != 0);
}

// メッセージ送信側 (親プロセス)
// 指定されたメッセージを子供に送る
void sender(int send_msgid, int chno, char *msg)
{
  int i;
  Msgbuf mbuf;
  int msgsize;

  mbuf.mtype = 100;
  for (i = 0; i < chno; i++) {
    snprintf(mbuf.mtext, MAXMSG, "%s", msg);
    msgsize = strlen(mbuf.mtext);
    if (msgsnd(send_msgid, &mbuf, msgsize, 0)) {
      perror("Message Send");
    }
    /* printf("Parent sends \"%s\"\n", mbuf.mtext); */
  }
}

void recv_hand(int recv_msgid, Hand hands[MAXCHNUM], int chnum)
{
  int i;
  Msgbuf mbuf;
  int msgsize;
  
  // メッセージキューからメッセージを受け取る
  for ( i = 0; i < chnum; i++ ) {
    if ((msgsize = msgrcv(recv_msgid, &mbuf, MAXMSG, 0, 0)) == -1) {
      perror("Message receive");
    } else {
      mbuf.mtext[msgsize] = '\0';
      // 子の手を配列に格納
      hands[mbuf.mtext[0] - '0'] = mbuf.mtext[2] - '0';
    }    
  }
}

// 人数の入力
int input_cpu_num(void)
{
  int num, i;
  char buff[BUFF];

  puts("CPUの人数を入力してください．(1以上 10未満)");
  puts("0 または q を入力すると終わります．");
  while ( 1 ) {
    if ( fgets(buff, BUFF, stdin) == NULL ) {
      return FALSE;
    }
    for ( i = 0; buff[i] != '\0'; i++ ) {
      // 入力が正しいとき
      if ( '1' <= buff[i] && buff[i] <= '9' ) {
        num = buff[i] - '0';
        return num;
      }
      // ゲームの終了
      if ( buff[i] == 'q' || buff[i] == '0' ) { return 0; }
    }
    puts("入力が正しくありません，1~9までの数値を入力してください．");    
    puts("0 または q を入力すると終わります．");
  }
  return 0;
}

// 手の入力
Bool input_hand(Hand *hand)
{
  char buff[BUFF];
  int i;
  puts("手を入力してください．qを入力するとゲームが終了します．");
  puts("グー:0    チョキ:1    パー:2");
  while ( 1 ) {
    if ( fgets(buff, BUFF, stdin) == NULL ) {
      return FALSE;
    }
    for ( i = 0; buff[i] != '\0'; i++ ) {
      // 入力が正しいとき
      if ( '0' <= buff[i] && buff[i] <= '2' ) {
        *hand = buff[i] - '0';
        return TRUE;
      }
      // ゲームの終了
      if ( buff[i] == 'q' ) { return FALSE; }
    }
    puts("入力が正しくありません，0~2までの数値を入力してください．");
  }
  return TRUE;
}

void cluc_result(Hand *hands, Result *results, int chnum)
{
  int i;
  Bool stone_flag = FALSE;
  Bool scissors_flag = FALSE;
  Bool paper_flag = FALSE;
  Bool draw_flag = TRUE;
  Hand winner_hand;
  

  // どんな手があったがの確認
  for ( i = 0; i < chnum; i++ ) {
    switch ( hands[i] ) {
    case STONE: stone_flag = TRUE; break;
    case SCISSORS: scissors_flag = TRUE; break;
    case PAPER: paper_flag = TRUE; break;
    }      
  }

  // 勝者の確認
  if ( stone_flag != TRUE && scissors_flag && paper_flag  ) { 
    winner_hand = SCISSORS; 
    draw_flag = FALSE;
  }
  if ( stone_flag && scissors_flag != TRUE && paper_flag  ) {
    winner_hand = PAPER;
    draw_flag = FALSE;
  }
  if ( stone_flag && scissors_flag && paper_flag != TRUE  ) {
    winner_hand = STONE;
    draw_flag = FALSE;
  }
  // 結果を配列に格納
  for ( i = 0; i < chnum; i++ ) {
    if ( draw_flag ) { 
      results[i] = DRAW;
    } else {
      if ( hands[i] == winner_hand ) { results[i] = WIN; }
      else { results[i] = LOSE; }
    }
  }
}

// 結果の出力
void print_result(Hand *hands, Result *results, int chnum)
{
  int i;
  for ( i = 0; i < chnum; i++ ) {
    printf("CPU    : %s\t: %s\n", g_hand_str[hands[i]], g_result_str[results[i]]);
  }
  printf("PLAYER : %s\t: %s\n", g_hand_str[hands[i]], g_result_str[results[i]]);
  puts("-----------------------------------------");
  printf("                 %s\n", g_result_str[results[i]]);
  puts("-----------------------------------------");
}

void init(Hand *hands, Result *results)
{
  int i;
  for ( i = 0; i < MAXCHNUM; i++ ) {
    hands[i] = STONE;
    results[i] = LOSE;
  }
}

int main()
{
  int pid[MAXCHNUM];
  int pno;
  int status;
  int i;
  int parent_msgid;
  int child_msgid;
  struct msqid_ds qds;
  int seeds[MAXCHNUM];
  Hand hand;
  Hand hands[MAXCHNUM + 1];  // 全てのプレイヤーの手を格納
  Result results[MAXCHNUM + 1];  // じゃんけんの結果を格納
  Bool flag;
  int cpu_num = 0;

  // 親が子にメッセージ送信で使う
  parent_msgid = msgget(IPC_PRIVATE, IPC_CREAT | 0666);
  if (parent_msgid == -1) {
    perror("msgget");
    exit(1);
  }
  if (msgctl(parent_msgid, IPC_STAT, &qds)) {
    perror("msgctl");
    exit(1);
  }
  /* printf("Change queue size %lu to %d\n",qds.msg_qbytes, QSIZE); */
  qds.msg_qbytes = QSIZE;
  if (msgctl(parent_msgid, IPC_SET, &qds)) {
    perror("msgctl");
    exit(1);
  }
  // 子が親にメッセージ送信で使う
  child_msgid = msgget(IPC_PRIVATE, IPC_CREAT | 0666);
  if (child_msgid == -1) {
    perror("msgget");
    exit(1);
  }
  if (msgctl(child_msgid, IPC_STAT, &qds)) {
    perror("msgctl");
    exit(1);
  }
  /* printf("Change queue size %lu to %d\n",qds.msg_qbytes, QSIZE); */
  qds.msg_qbytes = QSIZE;
  if (msgctl(child_msgid, IPC_SET, &qds)) {
    perror("msgctl");
    exit(1);
  }


  // 乱数の初期化
  srand((unsigned int)time(NULL));
  rand(); rand(); rand(); rand(); rand();

  // CPU 数の入力
  cpu_num = input_cpu_num();
  if ( cpu_num >= 1 ) { printf("%d 人で じゃんけん を行います．\n", cpu_num + 1); } 
  else { printf("%d 人なので じゃんけん できません．\n", cpu_num); }

  for (pno = 0; pno < cpu_num; pno++) {
    // 10000未満の乱数を生成
    seeds[pno] = rand() / (RAND_MAX + 1.0) * 10000;
    pid[pno] = fork();
    if (pid[pno] == -1) {
      printf("Fork error\n");
      break;
    } else if (pid[pno] == 0) {
      cpu_player(parent_msgid, child_msgid, pno, seeds[pno]);
      return pno;
    } else {
      /* printf("Process id of child process is %d\n",pid[pno]); */
    }
  }

  // 親プロセスの処理
  if (pno) {	// 子プロセスを1つ以上持っている
    while ( 1 ) {
      init(hands, results);
      // playerに手を入力させる
      flag = input_hand(&hand);
      // flag が FASLEであればゲームを強制終了
      if ( flag == FALSE ) {
        // 子に終了用のメッセージを送る
        sender(parent_msgid, cpu_num, ENDMSG);
        break;
      }
      // player の手を格納
      hands[cpu_num] = hand;
      // 子に手を決定するようにメッセージを送る
      sender(parent_msgid, cpu_num, DECMSG);
      // flag が trueの場合 じゃんけんを行う
      // 子の手を受け取る
      recv_hand(child_msgid, hands, cpu_num);
      // ジャッジする
      cluc_result(hands, results, cpu_num + 1);
      // 結果を出力する
      print_result(hands, results, cpu_num);
    }

    // 子の終了を待つ
    for (i = 0; i < pno; i++) {
      if (wait(&status) == -1) {
        perror("Wait error\n");
      } else {
        /* printf("Terminates Child #%d\n", (status >> 8) & 0xff); */
      }
    }
  }
  if (msgctl(parent_msgid, IPC_RMID, NULL)) {
    perror("shmctl");
  }
  if (msgctl(child_msgid, IPC_RMID, NULL)) {
    perror("shmctl");
  }
}
