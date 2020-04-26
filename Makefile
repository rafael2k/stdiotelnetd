CC = cc -Wall
APPNAME = stdiotelnetd
OBJS = main.o server.o connection.o ringbuf.o telnetd.o rawtty.o spawn.o
CFLAGS = -DDEBUG -DRINGBUF_CAPACITY=512U -DMAX_CONN=7U `pkg-config --cflags libtelnet`
LIBS = `pkg-config --libs libtelnet`

%.o: %.c
	$(CC) -c $< $(CFLAGS)

$(APPNAME): $(OBJS)
	$(CC) -o $(APPNAME) $(OBJS) $(LIBS)

clean:
	rm -f *.o
	rm -f $(APPNAME)
	rm -f core*
