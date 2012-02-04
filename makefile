CC=gcc
CFLAGS=-Wall -g 
HDRS=mythread.h queue.h futex.h
SRCS=futex.c mythread.c queue.c mythread_test.c
OBJS=futex.o mythread.o queue.o mythread_test.o
NAME=driver
AR=ar
LIB_NAME=mythreadlib.a
ARKEYS=r

all: $(OBJS)
	$(CC) $(OBJS) -o $(NAME)
futex.o: futex.h futex.c
mythread.o: mythread.h mythread.c futex.o queue.o
queue.o: queue.h queue.c
mythread_test.o: mythread_test.c futex.o queue.o mythread.o

.PHONY:clean ctags

clean:
	rm -rf $(OBJS) $(LIBNAME) $(NAME) tags
ctags:
	ctags -R *
bundle:
	$(AR) $(ARKEYS) $(LIB_NAME) $(OBJS)	
