all: niconico_server niconico_client

#OPENCV_CFLAGS = `pkg-config opencv-2.3.1 --cflags`
#OPENCV_LIBS   = `pkg-config opencv-2.3.1 --libs`
OPENCV_CFLAGS = `pkg-config opencv --cflags`
OPENCV_LIBS   = `pkg-config opencv --libs`
GLUT_CFLAGS   = `pkg-config gl --cflags` `pkg-config glu --cflags`
GLUT_LIBS     = `pkg-config gl --libs` `pkg-config glu --libs` -lglut

niconico_server: niconico_server.c
	gcc -pthread -o $@ $< ${OPENCV_CFLAGS} ${OPENCV_LIBS}
niconico_client: niconico_client.c
	gcc -pthread -o $@ $< ${OPENCV_CFLAGS} ${OPENCV_LIBS} ${GLUT_CFLAGS} ${GLUT_LIBS}
