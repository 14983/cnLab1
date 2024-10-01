#ifndef COMMON_H
#define COMMON_H

#define SERVER_PORT 4392
#define USER_SLEEP_TIME 200 // in milliseconds
#define SYSTEM_SLEEP_TIME 10 // in milliseconds
#define TIMEOUT 10000 // in milliseconds

#define INFO "\033[32m[INFO] \033[0m"
#define WARNING "\033[36m[WARN] \033[0m"
#define ERROR "\033[31m[ERRO] \033[0m"
#define THREAD "\033[33m[THRE] \033[0m"

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
#include <algorithm>
#include <csignal>

enum MSG_TYPE: int {
    REQ_TYPE_END_CONN,
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

#define COMFD_SIZE 4
#define IP_SIZE 4
#define PORT_SIZE 4

struct MSG {
/*
    | type | align     | length | data   |
    | 4B   | 4B(EMPTY) | 8B     | size B |

    size = size of 'data'

type = REQ_TYPE_END_CONN:
    client => server: request to end connection
    size = 0

type = REQ_TYPE_GET_TIME:
    client => server: request to get current time
    size = 0

type = REQ_TYPE_GET_NAME:
    client => server: request to get server's name
    size = 0

type = REQ_TYPE_GET_LIST:
    client => server: request to get list of connected clients
    size = 0

type = REQ_TYPE_SEND_MSG:
    client => server: request to send message to another client
    | comfd | message      |
    | 4B    | (size - 4)B  |

type = MSG_TYPE_GET_TIME:
    server => client: current time
    size = size of data, data = current time

type = MSG_TYPE_GET_NAME:
    server => client: server's name
    size = size of data, data = server's name

type = MSG_TYPE_GET_LIST:
    server => client: list of connected clients
    | number of clients | comfd1 | ip1 | port1 | comfd2 | ip2 | port2 | ... |
    | 4B                | 4B     | 4B  | 4B    | 4B     | 4B  | 4B    | ... |

type = MSG_TYPE_SEND_MSG:
    server => client: status of sending message
    size = 2
    data = '0\0': successful
           '1\0': failed
           '2\0': client not found

type = MSG_TYPE_RECV_MSG:
    server => client: received message from another client
    | comfd | ip | port | message       |
    | 4B    | 4B | 4B   | (size - 12)B  |
*/
    MSG_TYPE type;
    size_t size;
    char data[];
};

struct ADDRESS {
    char ip[INET_ADDRSTRLEN];
    int port;
    bool operator==(const ADDRESS &other) const {
        return (strcmp(ip, other.ip) == 0 && port == other.port);
    }
    ADDRESS() {}
    ADDRESS(const char _ip[], int _port): port(_port) {
        strncpy(ip, _ip, INET_ADDRSTRLEN - 1);
        ip[INET_ADDRSTRLEN - 1] = '\0';
    }
};

void create_socket(int &sockfd);
void close_socket(int &sockfd);
int get_message(int sockfd, MSG *&message); // the return value should be freed by the caller
int send_message(int sockfd, const MSG *message);
MSG *create_message(MSG_TYPE type); // the return value should be freed by the caller
MSG *create_message(MSG_TYPE type, const std::string &data); // the return value should be freed by the caller
MSG *create_message(MSG_TYPE type, size_t size, const char *data); // the return value should be freed by the caller
int blocked_send(int sockfd, const MSG *message);
int get_sockfd_status(int sockfd);

#endif
