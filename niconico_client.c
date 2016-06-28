//-----------------------------------------------------------------------------------
// vide_chat_client.c : ビデオチャットプログラム
//-----------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <arpa/inet.h>

#include <pthread.h>

#include <GL/glut.h>

#include <cv.h>
#include <highgui.h>

#include "defs.h"

static int g_socket;

static pthread_t g_tr, g_ts;
static pthread_mutex_t g_mutex;
static int g_width=640, g_height=480, g_depth=8, g_nChannel=3;
static int g_lena_width=640, g_lena_height=480, g_lena_depth=8, g_lena_nChannel=3;
static char g_image_buf[640*480*3];
static int g_switch = 1; //for logout

GLuint g_texture;

//-----------------------------------------------------------------------------------
// socket_open
//-----------------------------------------------------------------------------------
void
socket_open (const char* ip_addr)
{
  struct sockaddr_in sin;

  // ソケットを生成する
  //---------------------------------------------------
  if ((g_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("client: socket");
    exit(1);
  }

  // サーバの情報を与える
  //---------------------------------------------------
  sin.sin_family = AF_INET;  // アドレスの型の指定
  sin.sin_port = PORT;  // ポート番号
  sin.sin_addr.s_addr = inet_addr(ip_addr);  // 宛先のアドレスの指定 IPアドレスの文字列を変換

  // サーバに接続する
  //---------------------------------------------------
  if ((connect(g_socket, (struct sockaddr *)&sin, sizeof(sin))) < 0) {
    perror("client: connect");
    exit(1);
  }
}

//-----------------------------------------------------------------------------------
// socket_close
//-----------------------------------------------------------------------------------
void
socket_close()
{
  close( g_socket );
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

    if( size <= 0 ) break;
  }

  return buf_size - left_size;
}

//-----------------------------------------------------------------------------------
// send_sock
//-----------------------------------------------------------------------------------
void*
thread_send(void *arg)
{
  char buf[1024];
  printf("chat start\n");
  
  while(1){
    fgets(buf, sizeof(buf), stdin);
    buf[strlen(buf)-1] = '\0'; //改行文字消す
    int size = strlen(buf)+1;
    
    if ( strcmp(buf,"_logout") == 0 ) g_switch= 0;//for logout
    
    if (socket_write( g_socket, &size, sizeof(int) ) < 0){
      return NULL;
    }
    if (socket_write( g_socket, buf, size ) < 0){
      return NULL;
    }
  }
  
  return NULL;
}

//-----------------------------------------------------------------------------------
// recv_sock
//-----------------------------------------------------------------------------------
void*
thread_recv(void *arg)
{
  char buf[1024];
  int size;

  socket_read(g_socket, &g_width, sizeof(int) );
  socket_read(g_socket, &g_height, sizeof(int) );
  socket_read(g_socket, &g_depth, sizeof(int) );
  socket_read(g_socket, &g_nChannel, sizeof(int) );

  
  while(1){
    
    if(g_switch){
      socket_read(g_socket, &size, sizeof(int));
      // ダミーではないとき
      if( size > 0 ) {
	socket_read(g_socket, buf, size );      
	printf("%s\n", buf);
	
      } else {
	socket_read(g_socket, g_image_buf, g_width*g_height*(g_depth/8)*g_nChannel);
	//image_buf, width*nChannel
      }
    } else{ //after input logout
      socket_read(g_socket, &size, sizeof(int));
      if (size == 0){
	socket_read(g_socket, g_image_buf, size);
	break;
	  }
      else socket_read(g_socket, buf, size );
    }  
  }
  socket_close();
  return NULL;
}

//-----------------------------------------------------------------------------------
// init
//-----------------------------------------------------------------------------------
void init(void)
{
    glClearColor (0.0, 0.0, 0.0, 0.0);
    glShadeModel (GL_FLAT);

    glGenTextures( 1, &g_texture );
    glBindTexture( GL_TEXTURE_2D, g_texture );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 
}


//-----------------------------------------------------------------------------------
// display
//-----------------------------------------------------------------------------------
void display(void)
{
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-10, 10, -10, 10, -10, 10);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glColor3ub(255,255,255);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, g_texture);

    glScalef(20,20,20);
    glTranslatef(-0.5f, -0.5f, 0.0f);
    glBegin(GL_QUADS);
    glTexCoord2f(0,1);    glVertex2f(0,0);
    glTexCoord2f(1,1);    glVertex2f(1,0);
    glTexCoord2f(1,0);    glVertex2f(1,1);
    glTexCoord2f(0,0);    glVertex2f(0,1);
    glEnd();
    
    glColor3f(1.0,1.0,0.0);
    
    glFlush();
    glFinish();
    glutSwapBuffers();
}

//-----------------------------------------------------------------------------------
// idle
//-----------------------------------------------------------------------------------
void idle()
{
  
  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, g_width, g_height, 0,
		GL_BGR, GL_UNSIGNED_BYTE, g_image_buf );
  glutPostRedisplay();
}

//-----------------------------------------------------------------------------------
// reshape
//-----------------------------------------------------------------------------------
void reshape (int w, int h)
{
    glViewport(0, 0, w, h);
}


//-----------------------------------------------------------------------------------
// keyboard
//-----------------------------------------------------------------------------------
/* ARGSUSED1 */
void keyboard(unsigned char key, int x, int y)
{
    switch (key) {
    case 27:
	socket_close();
	exit(0);
	break;
    }
}

//-----------------------------------------------------------------------------------
// main
//-----------------------------------------------------------------------------------
int
main(int argc, char *argv[])
{
  
  if(argc == 1){
    printf("usage: client <server IP address>\n");
    exit(1);
  }

  // GLUTの初期化
  //---------------------------------------------------
  glutInit(&argc, argv);
  glutInitDisplayMode (GLUT_SINGLE | GLUT_RGB);
  glutInitWindowSize (680, 480);
  glutInitWindowPosition (100, 100);

  // サーバーへ接続
  //---------------------------------------------------
  socket_open( argv[1] );

  pthread_mutex_init( &g_mutex, NULL );
  pthread_create( &g_tr, NULL, thread_recv, NULL );
  pthread_create( &g_ts, NULL, thread_send, NULL );
  
  // 新しくウィンドウを作る
  //---------------------------------------------------
  glutCreateWindow (argv[0]);
  init();

  // GLUTのメインループ
  //---------------------------------------------------
  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyboard);
  glutIdleFunc(idle);
  glutMainLoop();

  return 0;
}
