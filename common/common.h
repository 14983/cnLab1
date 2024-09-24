#define SERVER_PORT 4392
#define USER_SLEEP_TIME 200 // in milliseconds
#define SYSTEM_SLEEP_TIME 10 // in milliseconds
#define TIMEOUT 10000 // in milliseconds

#define INFO "\033[32m[INFO] \033[0m"
#define WARNING "\033[36m[WARN] \033[0m"
#define ERROR "\033[31m[ERRO] \033[0m"
#define COMMUNICATION "\033[33m[COMM] \033[0m"

#include <iostream>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <unistd.h>
#include <poll.h>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <queue>
#include <chrono>
#include <cassert>

enum MSG_TYPE: int {
    MSG_TYPE_END_CONN,
    REQ_TYPE_GET_TIME,
    MSG_TYPE_GET_TIME,
    REQ_TYPE_GET_NAME,
    MSG_TYPE_GET_NAME,
    REQ_TYPE_GET_LIST,
    MSG_TYPE_GET_LIST,
    REQ_TYPE_SEND_MSG,
    MSG_TYPE_SEND_MSG,
    MSG_TYPE_RECV_MSG
};

struct MSG {
    MSG_TYPE type;
    size_t size;
    char data[];
};

struct ADDRESS {
    int sockfd;
    char ip[INET_ADDRSTRLEN];
    int port;
    bool operator==(const ADDRESS &other) const {
        return (strcmp(ip, other.ip) == 0 && port == other.port && sockfd == other.sockfd);
    }
};

class CONN_LIST {
public:
private:
};

void create_socket(int &sockfd);
void close_socket(int &sockfd);
int get_message(int sockfd, MSG *&message); // the return value should be freed by the caller
int send_message(int sockfd, const MSG *message);
MSG *create_message(MSG_TYPE type); // the return value should be freed by the caller
MSG *create_message(MSG_TYPE type, const std::string &data); // the return value should be freed by the caller
MSG *create_message(MSG_TYPE type, size_t size, const char *data); // the return value should be freed by the caller