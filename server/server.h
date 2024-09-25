#ifndef SERVER_H
#define SERVER_H

#include "../common/common.h"

class CONN_LIST {
public:
    int comfd;
    ADDRESS clientAddr;
    bool isConnected;
    CONN_LIST(): comfd(-1), clientAddr(ADDRESS()), isConnected(false) {}
    CONN_LIST(int _comfd, ADDRESS _addr, bool _isCon): comfd(_comfd), clientAddr(_addr), isConnected(_isCon) {}
    bool operator==(const CONN_LIST &other) const {
        return (comfd == other.comfd && clientAddr == other.clientAddr);
    }
};

int Bind(int &sockfd, const int port) {
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(serverAddr.sin_zero), 8);
    if(bind(sockfd, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
        return -1;
    return 0;
}

int acceptNewConnection(int sockfd, int &comfd, ADDRESS &clientAddr) {
    sockaddr_in rawClientAddr;
    socklen_t clientSize = sizeof(rawClientAddr);
    comfd = accept(sockfd, (sockaddr*)&rawClientAddr, &clientSize);
    if (comfd < 0) {
        return -1;
    }
    strcpy(clientAddr.ip, inet_ntoa(rawClientAddr.sin_addr));
    clientAddr.port = ntohs(rawClientAddr.sin_port);
    return 0;
}

#endif // SERVER_H