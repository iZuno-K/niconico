//g_で始まるものは必ずグローバル変数
//g_ついてないけどグローバル変数なのもある
//-----------------------------------------------------------------------------------
// video_chat_server.c : ビデオチャットプログラム
//-----------------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <pthread.h>
#include <signal.h>
#include <errno.h>

#include <cv.h>
#include <highgui.h>

#include "defs.h"
#include <time.h>

int socket_list[16];
int client_num = 0;
char messages[16][1024];

//to wirte character on image
int font_face = CV_FONT_HERSHEY_TRIPLEX;
CvFont font; //main関数内で初期化
static double g_char_x[16];
CvScalar rcolor;

int g_switch[4];
double g_warota_point[15];
int g_speed[15];
int g_min_place;

double g_danmaku1_point[15];
double g_danmaku2_point[15];
double g_danmaku3_point[15];

static IplImage* g_frame, *g_lena;
static pthread_mutex_t g_mutex;

//minを求める
int min_index(int a[]){
  int size = sizeof(a);
  int min_index = 0;
  int i;
  for (i=1; i<size; i++){
    if  (a[i] < a[min_index] ) {
      min_index = i;
    }
  }
  return min_index;
}

//-----------------------------------------------------------------------------------
// socket_read
//-----------------------------------------------------------------------------------
int
socket_read( int socket, void* buf, int buf_size )
{
  int size;
  int left_size = buf_size;
  char* p = (char*) buf;

  // 指定されたサイズのバッファを受信完了するまで、
  // ループで読み込み続ける
  //---------------------------------------------------
  while( left_size > 0 ) {
    size = read(socket, p, left_size);
    p += size;
    left_size -= size;

    if( size <= 0 ) break;
  }

  return buf_size - left_size;
}

//-----------------------------------------------------------------------------------
// socket_write
//-----------------------------------------------------------------------------------
int
socket_write( int socket, void* buf, int buf_size )
{
  int s;
  int size;
  int left_size = buf_size;
  char* p = (char*) buf;

  // 指定されたサイズのバッファを送信完了するまで、
  // ループで書き込み続ける
  //---------------------------------------------------
  while( left_size > 0 ) {
    size = write(socket, p, left_size);
    p += size;
    left_size -= size;


    if( size == 0 ) break;
    if( size <  0 ) {
      switch( errno ){
      case EPIPE:
        for(s = 0; s < client_num; s++) {
          if ( socket_list[s] == socket ) {
            int j;
            for ( j = s; j < client_num-1; j++) {
              socket_list[j] = socket_list[j+1];
            }
            client_num--;
          }
        }
        printf("Disconnected : %d\n", socket);
        return -1;
      }
      break;
    }
  }

  return buf_size - left_size;
}

//-----------------------------------------------------------------------------------
// kusowarota
//-----------------------------------------------------------------------------------
void kusowarota(IplImage* img){
  int i;
  char kusowarota[] = "wwwwwwwwwwwwwwwwwwwww";
  for (i=0; i<15; i++){    
    cvPutText (img, kusowarota, cvPoint ( g_warota_point[i], (i+1)*30), &font, rcolor);
    g_warota_point[i] -= g_speed[i];
  }
  if (g_warota_point[g_min_place] <= -23*30) { //左端に消えきった時(文字列長*30速いから増やしてもいい)
    g_switch[0] = 0;
    //表示位置初期化
    for (i=0;i<15;i++){
      g_warota_point[i] = 645;
    }
  }
}

//-----------------------------------------------------------------------------------
// dammaku1
//-----------------------------------------------------------------------------------
void danmaku1 (IplImage* img){
  CvScalar yellow;
  yellow = CV_RGB (0, 255, 100);
  CvFont font1;
  cvInitFont (&font1, font_face, 0.3, 0.5, 0, /*thickness*/ 1, 8);
  
  char danmaku_1[15][100] = {
    "***>>>===---  ",
    "***>>>===---  ",
    "***>>>===--- +++>>>===--- ",
    "***>>>===--- +++>>>===--- ", 
    "***>>>===--- +++>>>===--- ***>>>===--- ",
    "***>>>===--- +++>>>===--- ***>>>===--- ",
    "***>>>===--- +++>>>===--- ***>>>===--- +++>>>===--- ",
    "***>>>===--- +++>>>===--- ***>>>===--- +++>>>===--- ",
    "***>>>====----~~~~ +++>>>====----~~~~ ***>>>====----~~~~ +++>>>====----~~~~ ",
    "***>>>====----~~~~ +++>>>====----~~~~ ***>>>====----~~~~ +++>>>====----~~~~ ",
    "***>>>====----~~~~ +++>>>====----~~~~ ***>>>====----~~~~ +++>>>====----~~~~ "
  };
  int size = 100;//だいたい最大文字列長
  int i,j;
  
  for (i=0; i<11 ; i ++ ){
    for (j=0;j<3;j++){
      cvPutText (img, danmaku_1[i], cvPoint ( g_danmaku1_point[i], (i*3+j+1)*15), &font1, yellow);
    }
    g_danmaku1_point[i] -= 8;//g_speed[i];
  }  
  if (g_danmaku1_point[g_min_place] <= -size*15) { //左端に消えきった時(文字列長*文字の大きさ15)
    g_switch[1] = 0;
    //表示位置初期化
    for (i=0;i<15;i++){
      g_danmaku1_point[i] = 640;
    }
  }
}


//-----------------------------------------------------------------------------------
// dammaku2
//-----------------------------------------------------------------------------------
void danmaku2 (IplImage* img){
  char danmaku_2[15][100] = {
    "(^_^)/ (^_^)/ (^_^)/ (^_^)/ (^_^)/ (^_^)/ (^_^)/ (^_^)/ (^_^)/ (^_^)/",
    "?(>.<)? ?(>.<)? ?(>.<)? ?(>.<)? ?(>.<)? ?(>.<)? ?(>.<)? ?(>.<)? ?(>.<)? ", 
    "($_$) ($_$) ($_$) ($_$) ($_$) ($_$) ($_$) ($_$) ($_$) ($_$) ($_$) ($_$) ",
    "(^_^)/ ?(>.<)? (^_^)/ ?(>.<)? (^_^)/ ?(>.<)? (^_^)/ ?(>.<)? (^_^)/ ?(>.<)? ",
    "(^_^)/ ?(>.<)? (^_^)/ ?(>.<)? (^_^)/ ?(>.<)? (^_^)/ ?(>.<)? (^_^)/ ?(>.<)? ",
    "(^_^)/ ?(>.<)? (^_^)/ ?(>.<)? (^_^)/ ?(>.<)? (^_^)/ ?(>.<)? (^_^)/ ?(>.<)? ",
    "(^_^)/ ?(>.<)? ($_$) (^_^)/ ?(>.<)? ($_$) (^_^)/ ?(>.<)? ($_$) (^_^)/ ?(>.<)? ",
    "(^_^)/ ?(>.<)? ($_$) (^_^)/ ?(>.<)? ($_$) (^_^)/ ?(>.<)? ($_$) (^_^)/ ?(>.<)? ",
    "(^_^)/ ?(>.<)? ($_$) (^_^)/ ?(>.<)? ($_$) (^_^)/ ?(>.<)? ($_$) (^_^)/ ?(>.<)? ",
    "?(>.<)? ($_$) ?(>.<)? ($_$) ?(>.<)? ($_$) ?(>.<)? ($_$) ?(>.<)? ($_$) ?(>.<)? ",
    "?(>.<)? ($_$) ?(>.<)? ($_$) ?(>.<)? ($_$) ?(>.<)? ($_$) ?(>.<)? ($_$) ?(>.<)? ",
    "?(>.<)? ($_$) ?(>.<)? ($_$) ?(>.<)? ($_$) ?(>.<)? ($_$) ?(>.<)? ($_$) ?(>.<)? ",
    "($_$) ($_$) ($_$) ($_$) ($_$) ($_$) ($_$) ($_$) ($_$) ($_$) ($_$) ($_$) ",
    "?(>.<)? ?(>.<)? ?(>.<)? ?(>.<)? ?(>.<)? ?(>.<)? ?(>.<)? ?(>.<)? ?(>.<)? ",
    "(^_^)/ (^_^)/ (^_^)/ (^_^)/ (^_^)/ (^_^)/ (^_^)/ (^_^)/ (^_^)/ (^_^)/",
  };
  int size = 100;//だいたい最大文字列長
  int i;
  CvScalar blue;
  blue = CV_RGB (0, 100, 255);
  CvFont font2;

  cvInitFont (&font2, font_face, 1.0, 1.0, 0, /*thickness*/ 1, 8);
  for (i=0; i<15; i++){    
    cvPutText (img, danmaku_2[i], cvPoint ( g_danmaku2_point[i], (i+1)*30), &font2, blue);
    g_danmaku2_point[i] -= 10;
  }  
  if (g_danmaku2_point[14] <= -24*size) { //左端に消えきった時(文字列長*24)
    g_switch[2] = 0;
    //表示位置初期化
    for (i=0;i<15;i++){
      g_danmaku2_point[i] = 640;
    }
  }
}

//-----------------------------------------------------------------------------------
// dammaku3
//-----------------------------------------------------------------------------------
void danmaku3 (IplImage* img){
  char danmaku_3[15][100] = {
    "11111111111111111111111111111111111111111111111111111111111111111111111111",
    "11111111111111111111111111111111111111111111111111111111111111111111111111", 
    "11111111188811111111888881111111111111111111111111188888111111118881111111",
    "11111111888111111188888111188811111111111111188811111888881111111888111111",
    "11111118881111118881111111188811111111111111188811111111188811111188811111",
    "11111188811111111111111111188811111111111111188811111111111111111118881111",
    "11111888111111111111111111118888888888888888888111111111111111111111888111",
    "11111888111111111111111111111888111111111118881111111111111111111111888111",
    "11111888111111111111111111111188811111111188811111111111111111111111888111",
    "11111888111111111111111111111118881111111888111111111111111111111111888111",
    "11111888111111111111111111111111888111118881111111111111111111111111888111",
    "11111188811111111111111111111111188811188811111111111111111111111118881111",
    "11111118881111111111111111111111118881881111111111111111111111111188811111",
    "11111111888111111111111111111111111188811111111111111111111111111888111111",
    "11111111111111111111111111111111111111111111111111111111111111111111111111"
  };
  int size = 100;//およそ文字列長100
  int i;
  CvFont font3;
  CvScalar red;
  
  red = CV_RGB (255, 50, 50);
  //font2初期化：大きさ小さめに
  cvInitFont (&font3, font_face, 0.3, 0.5, 0, /*thickness*/ 2, 8);
  
  for (i=0; i<15; i++){    
    cvPutText (img, danmaku_3[i], cvPoint ( g_danmaku3_point[i], (i+15)*13), &font3, red);
    g_danmaku3_point[i] -= 5;
  }  
  if (g_danmaku3_point[14] <= -15*size) { //左端に消えきった時(文字列長*文字大きさ15)
    g_switch[3] = 0;
    //表示位置初期化
    for (i=0;i<15;i++){
      g_danmaku3_point[i] = 640;
    }
  }
}

//-----------------------------------------------------------------------------------
// commands　コマンド集
//-----------------------------------------------------------------------------------
void commands(IplImage* img){
  //command
  char c[4][50] = {"_kusowarota", "_danmaku1", "_danmaku2", "_danmaku3"};
  int i,j;
  for (i=0; i<client_num; i++){
    for (j=0; j<4; j++){
      if (strcmp(c[j], messages[i]) == 0) {
	g_switch[j] = 1;
      }
    }
  }
  //excute command
  if(g_switch[1]){
    danmaku1(img);
  }
  if(g_switch[2]){
    danmaku2(img);
  }
  if(g_switch[3]){
    danmaku3(img);
  }
  if(g_switch[0]) {
    kusowarota(img);  
  }//kusowarotaは最後にかぶせたい
  
} 

//-----------------------------------------------------------------------------------
// write_char_img: 画像に文字合成
//-----------------------------------------------------------------------------------

void write_char_img (IplImage* img)
{
  int i;
  //int size = strlen(buf);
  
  //上-strlen()*19で完全に左端に消える
  for (i=0; i<client_num; i++){    
    cvPutText (img, messages[i], cvPoint ( g_char_x[i], (i+1)*30), &font, rcolor);
    g_char_x[i] -= 7;
  }

}

//-----------------------------------------------------------------------------------
// comm：クライアントごとの送受信スレッド
//-----------------------------------------------------------------------------------
void*
comm(void* arg)
{
    /* pthread_create(&thread, NULL, comm, (void*)&ns); で作成したスレッド
       から、呼び出される */
  int s = *(int *)arg;
  int size;
  char buf[1024];
  pthread_t char_thread;
  int new_client_index;

  pthread_detach(pthread_self());
  
  pthread_mutex_lock( &g_mutex );
  {
    new_client_index = client_num;
    socket_list[new_client_index] = s;
    client_num++;
  }
  pthread_mutex_unlock( &g_mutex );

  while(1){
    int i, ret;

    ret = socket_read(s, &size, sizeof(int));
    if ( ret <= 0 ) return NULL;

    if( size > 0 ) {

      pthread_mutex_lock( &g_mutex );
      {
        /*担当するクライアントから受信する*/
	socket_read(s, buf, size);
	//logout
	if ( strcmp(buf, "_logout") == 0 ){
	  int size2 = 0;
	  socket_write(s, &size2, sizeof(int));
	  socket_write(s, g_lena->imageData, g_lena->width*g_lena->height*(g_lena->depth/8)*g_lena->nChannels);
	  memset(buf, '\0', strlen(buf)+1);
	    }

	printf("Client%d: %s\n",new_client_index,buf);
	
	memset( messages[new_client_index], '\0',  strlen(buf)+1);
	strcpy( messages[new_client_index], buf);
	g_char_x[new_client_index] = 640;
	
        /*全てのクライアントに受信データ送信する*/
	for(i = 0; i < client_num; i++) {
	  //受信元にはおくらない
	  if (socket_list[i] == s) continue;
	  
	  if ( socket_write( socket_list[i], &size, sizeof(int) )  < 0 ) {
            return NULL;
          }
          if ( socket_write( socket_list[i], buf, size ) < 0 ) {
            return NULL;
          }
	}
      }
      pthread_mutex_unlock( &g_mutex );
    }

  }

  return NULL;
}

//-----------------------------------------------------------------------------------
// thread_idle：アイドル時
//-----------------------------------------------------------------------------------
void*
thread_idle(void* arg)
{
  int size = 0;
  CvCapture *capture = (CvCapture *)arg;

  char buf[] = "Sample Opencv Test";

  pthread_detach(pthread_self());

  cvNamedWindow("Server", CV_WINDOW_AUTOSIZE);
  while(1){
    int i;
    
    IplImage*frame = cvQueryFrame(capture);
    
    if (frame == NULL) {
      if (cvGetCaptureProperty(capture,CV_CAP_PROP_POS_AVI_RATIO) > 0.9)
        cvSetCaptureProperty(capture,CV_CAP_PROP_POS_AVI_RATIO,0.0);
      continue;
    }

    commands(frame);
    //wirte char on frame
    write_char_img(frame);
    cvShowImage("Server", frame);
    
    if ( cvWaitKey(10) == 27 ){
      cvReleaseCapture(&capture);
      printf("Relesase success\n");
      cvDestroyWindow("Server");
      exit(1);
    }
      
    
    // すべての接続中のクライアントへバッファを送信
    //---------------------------------------------------
    pthread_mutex_lock( &g_mutex );
    {
      for(i = 0; i < client_num; i++) {
        socket_write( socket_list[i], &size, sizeof(int) );
        socket_write( socket_list[i], frame->imageData, frame->width*frame->height*(frame->depth/8)*frame->nChannels);
      }
    }
    pthread_mutex_unlock( &g_mutex );
  }
  cvDestroyWindow("Server");

  return NULL;
}

//-----------------------------------------------------------------------------------
// main
//-----------------------------------------------------------------------------------
int
main(int argc, char *argv[])
{
  CvCapture *capture = NULL;
  IplImage *frame;
  int i, s, ns;
  struct sockaddr_in sin;
  pthread_t thread;

  signal( SIGPIPE , SIG_IGN );

  if (argc == 1
      || (argc == 2 && strlen(argv[1]) == 1 && isdigit(argv[1][0])))
    capture = cvCreateCameraCapture(argc == 2 ? argv[1][0] - '0' : 0);
  else if (argc == 2 )
    capture = cvCreateFileCapture(argv[1]);

  if (capture==NULL) {
    fprintf(stderr, "ERROR : could not create capture object\n");
    exit(1);
  }

  for (i=0;i<3;i++) frame = cvQueryFrame(capture);
  
  //フォント構造体を初期化する(write_char_img用)
  cvInitFont (&font, font_face, 1.0, 1.0, 0, /*thickness*/ 1, 8);
  rcolor = CV_RGB (255, 255, 255);
  srand((unsigned)time(NULL));
  for (i=0; i< 15 ; i++) {
    g_warota_point[i] = 645;
    g_speed[i] = rand()%15 + 10;
    g_danmaku1_point[i] = 645;
    g_danmaku2_point[i] = 645;
    g_danmaku3_point[i] = 645;
  }
  g_min_place = min_index(g_speed);
  
  //logout時送る画像
  g_lena = cvLoadImage ("Lena_See_you.png", CV_LOAD_IMAGE_COLOR);
  
  // ソケットの生成
  //---------------------------------------------------
  if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("server: socket");
    exit(1);
  }

  memset( (char *)&sin, 0, sizeof(struct sockaddr) );

  sin.sin_family = AF_INET; // アドレスの型の指定
  sin.sin_port = PORT; // ポート番号
  sin.sin_addr.s_addr = INADDR_ANY;
//inet_addr("10.1.1.38"); // 待ち受けのIPアドレスの設定

  /* ソケットにパラメータを与える */
  if ((bind(s, (struct sockaddr *)&sin, sizeof(sin))) < 0) {
    perror("server: bind");
    exit(1);
  }

  // クライアントの接続を待つ
  //---------------------------------------------------
  if ((listen(s, 10)) < 0) {
    perror("server: listen");
    exit(1);
  }

  printf("waiting for connection\n");

  // thread_idle（アイドル時）用のスレッドを作成
  pthread_create(&thread, NULL, thread_idle, (void*)capture);

  // クライアントからの接続あれば、スレッドを生成する
  //---------------------------------------------------
  while(1){
    if ((ns = accept(s, NULL, 0)) < 0) {
      perror("server: accept");
      continue;
    }

    printf("Connected : %d\n", ns);

    socket_write( ns, &frame->width, sizeof(int) );
    socket_write( ns, &frame->height, sizeof(int) );
    socket_write( ns, &frame->depth, sizeof(int) );
    socket_write( ns, &frame->nChannels, sizeof(int) );

    //comm用のスレッドを生成する
    pthread_create(&thread, NULL, comm, (void*)&ns);
  }

  cvReleaseCapture(&capture);
  printf("Relesase success\n");
}
