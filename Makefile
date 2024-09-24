CC=g++
ENV=gnome-terminal --window --command

main: client.out server.out

client.out: client.o common.o
	$(CC) client.o common.o -o client.out

server.out: server.o common.o
	$(CC) server.o common.o -o server.out

common.o: common/common.cpp
	$(CC) -c common/common.cpp -o common.o

client.o: client/main.cpp common/common.h
	$(CC) -c client/main.cpp -o client.o

server.o: server/main.cpp common/common.h
	$(CC) -c server/main.cpp -o server.o

run: client.out server.out
	$(ENV) "./server.out"
	$(ENV) "./client.out"

clean:
	rm -f *.out *.o