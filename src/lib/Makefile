PROGRAM = libums.so
SRC = ums.c completion_list.c worker_thread.c
OBJS = ums.o completion_list.o worker_thread.o

all:
	gcc -Wall -I../lkm -c $(SRC)
	gcc -Wall -shared -fPIC -o $(PROGRAM) $(OBJS) -lpthread

clean:
	rm -v $(OBJS) $(PROGRAM)