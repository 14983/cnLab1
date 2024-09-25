#include "common.h"

void create_socket(int &sockfd) {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) return;
    int flags = fcntl(sockfd, F_GETFL, 0); // set to non-blocking
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}

void close_socket(int &sockfd) {
    close(sockfd);
    std::cout << INFO << "socket closed, fd = " << sockfd << std::endl;
}

MSG *create_message(MSG_TYPE type) { // the return value should be freed by the caller
    MSG *msg = (MSG *)malloc(sizeof(MSG));
    memset(msg, 0, sizeof(MSG));
    msg->type = type;
    msg->size = 0;
    return msg;
}

MSG *create_message(MSG_TYPE type, const std::string &data) { // the return value should be freed by the caller
    size_t size = data.size();
    MSG *msg = (MSG *)malloc(sizeof(MSG) + size + 1);
    memset(msg, 0, sizeof(MSG) + size + 1);
    msg->type = type;
    msg->size = size + 1;
    memcpy(msg->data, data.c_str(), size);
    msg->data[size] = '\0';
    return msg;
}

MSG *create_message(MSG_TYPE type, size_t size, const char *data) { // the return value should be freed by the caller
    MSG *msg = (MSG *)malloc(sizeof(MSG) + size);
    memset(msg, 0, sizeof(MSG) + size);
    msg->type = type;
    msg->size = size;
    memcpy(msg->data, data, size);
    return msg;
}

int get_message(int sockfd, MSG *&msg) { // the return value should be freed by the caller
    // note: recv() will **not** be blocked.
    // return value: 1 for success, 0 for no message, negative for error
    MSG tmp_msg;
    int ret = 0;
    bool is_header_received = false;
    char *target = (char *)&tmp_msg;
    int nLeft = sizeof(MSG);
    while (nLeft > 0) {
        ret = recv(sockfd, target, nLeft, 0);
        if (ret <= 0) {
            if (!is_header_received){
                return ret;
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(SYSTEM_SLEEP_TIME));
            }
        }
        is_header_received = true;
        nLeft -= ret;
        target += ret;
    }
    nLeft = tmp_msg.size;
    msg = (MSG *)malloc(sizeof(MSG) + tmp_msg.size);
    memcpy(msg, &tmp_msg, sizeof(MSG));
    target = (char *)(msg) + sizeof(MSG);
    while (nLeft > 0) {
        ret = recv(sockfd, target, nLeft, 0);
        if (ret <= 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(SYSTEM_SLEEP_TIME));
        }
        nLeft -= ret;
        target += ret;
    }
    return 1;
}

int send_message(int sockfd, const MSG *message) {
    // note: send() will **not** be blocked.
    // return value: 1 for success, negative for error
    char *serialized_message = (char *)message;
    size_t message_size = sizeof(MSG) + message -> size;
    int nLeft = message_size;
    int ret = 0;
    bool is_header_sent = false;
    while (nLeft > 0) {
        ret = send(sockfd, serialized_message, nLeft, 0);
        if (ret <= 0) {
            if (!is_header_sent){
                return ret;
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(SYSTEM_SLEEP_TIME));
            }
        }
        is_header_sent = true;
        nLeft -= ret;
        serialized_message += ret;
    }
    return 1;
}