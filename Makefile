CC=g++
GTKFLAG=`pkg-config --cflags --libs gtk+-3.0` 
ENV=gnome-terminal --window --command

main: client.out server.out

client.out: client.o common.o
	$(CC) client.o common.o -o client.out $(GTKFLAG) 

server.out: server.o common.o
	$(CC) server.o common.o -o server.out $(GTKFLAG) 

common.o: common/common.cpp
	$(CC) -c common/common.cpp -o common.o $(GTKFLAG) 

client.o: client/main.cpp common/common.h common/notify.h
	$(CC) -c client/main.cpp -o client.o $(GTKFLAG) 

server.o: server/main.cpp common/common.h
	$(CC) -c server/main.cpp -o server.o $(GTKFLAG) 

run: client.out server.out
	$(ENV) "./server.out"
	$(ENV) "./client.out"

clean:
	rm -f *.out *.o