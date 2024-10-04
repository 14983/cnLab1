#include "../common/common.h"
#include "server.h"

std::mutex clientMutex;
std::mutex coutMutex;
std::mutex killMutex;
std::vector<CONN_LIST> clients;

int sockfd;

bool killServer = false;

std::string getHostName() {
    char hostname[1024];
    gethostname(hostname, 1024);
    return std::string(hostname);
}

void signalHandler(int signum) {
    {
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cout << "Received signal (" << signum << "). shutting down server..." << std::endl;
    }
    {
        std::lock_guard<std::mutex> lock(killMutex);
        killServer = true;
    }
    close_socket(sockfd);
}

void handleClient(int comfd, ADDRESS clientAddr) {
    bool isEnd = false;
    while(isEnd == false) {
        {
            std::lock_guard<std::mutex> lock(killMutex);
            if(killServer) {
                break;
            }
        }
        MSG *msg = nullptr;
        if(get_message(comfd, msg) != 1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(SYSTEM_SLEEP_TIME));
            continue;
        }
        {
            std::lock_guard<std::mutex> lock(coutMutex);
            std::cout << "Received message from " << clientAddr.ip << ": " << clientAddr.port << " - " << msg->type << std::endl;
        }
        switch(msg->type) {
            case REQ_TYPE_END_CONN: {
                isEnd = true;
                break;
            }
            case REQ_TYPE_GET_TIME: {
                time_t now = time(0);
                std::string timeStr = ctime(&now);
                MSG *response = create_message(MSG_TYPE_GET_TIME, timeStr);
                blocked_send(comfd, response);
                free(response);
                {
                    std::lock_guard<std::mutex> lock(coutMutex);
                    std::cout << "Get current time and send" << std::endl;
                }
                break;
            }
            case REQ_TYPE_GET_NAME: {
                MSG *response = create_message(MSG_TYPE_GET_NAME, getHostName());
                send_message(comfd, response);
                free(response);
                {
                    std::lock_guard<std::mutex> lock(coutMutex);
                    std::cout << "Get server name and send" << std::endl;
                }
                break;
            }
            case REQ_TYPE_GET_LIST: {
                std::lock_guard<std::mutex> lock(clientMutex);
                uint32_t len = clients.size(), offset = 0;
                uint32_t _size = 4 + len * (COMFD_SIZE + IP_SIZE + PORT_SIZE);
                char* data = (char*)malloc(_size);
                memcpy(data, &len, 4);
                offset += 4;
                for(auto &client: clients) {
                    memcpy(data + offset, &client.comfd, COMFD_SIZE);
                    offset += COMFD_SIZE;
                    in_addr_t addr = inet_addr(client.clientAddr.ip);
                    memcpy(data + offset, &addr, IP_SIZE);
                    offset += IP_SIZE;
                    memcpy(data + offset, &client.clientAddr.port, PORT_SIZE);
                    offset += PORT_SIZE;
                }
                {
                    std::lock_guard<std::mutex> lock(coutMutex);
                    std::cout << "Get list of connected clients" << std::endl;
                    std::cout << "Number of clients: " << len << std::endl;
                    for(auto &client: clients) {
                        std::cout << "Client: " << client.clientAddr.ip << ": " << client.clientAddr.port << ", comfd = " << client.comfd << std::endl;
                    }
                }
                MSG *response = create_message(MSG_TYPE_GET_LIST, _size, data);
                send_message(comfd, response);
                free(data);
                free(response);
                {
                    std::lock_guard<std::mutex> lock(coutMutex);
                    std::cout << "Send list of connected clients" << std::endl;
                }
                break;
            }
            case REQ_TYPE_SEND_MSG: {
                int32_t _toComfd;
                memcpy(&_toComfd, msg->data, COMFD_SIZE);
                {
                    std::lock_guard<std::mutex> lock(coutMutex);
                    std::cout << "Send message to " << _toComfd << "  : "  << msg->data << std::endl;
                }
                std::lock_guard<std::mutex> lock(clientMutex);
                bool _isSended = false;
                for(auto &client: clients) {
                    if(client.comfd == _toComfd) {
                        std::string _data = "";
                        _data.insert(0, msg->data + COMFD_SIZE, msg->size - COMFD_SIZE);
                        _data.insert(0, (char*)&clientAddr.port, PORT_SIZE);
                        uint32_t ip = inet_addr(clientAddr.ip);
                        _data.insert(0, (char*)&ip, IP_SIZE);
                        _data.insert(0, (char*)&comfd, COMFD_SIZE);
                        MSG *forwardMsg = create_message(MSG_TYPE_RECV_MSG, _data);
                        if(send_message(_toComfd, forwardMsg) == -2) {
                            clients.erase(std::remove(clients.begin(), clients.end(), client), clients.end());
                            close_socket(_toComfd);
                            break;
                        }
                        MSG *response = create_message(MSG_TYPE_SEND_MSG, 1, "0");
                        send_message(comfd, response);
                        free(response);
                        free(forwardMsg);
                        _isSended = true;
                        {
                            std::lock_guard<std::mutex> lock(coutMutex);
                            std::cout << "Send message to " << client.clientAddr.ip << ": " << client.clientAddr.port << std::endl;
                        }
                        break;
                    }
                }
                if(!_isSended) {
                    MSG *response = create_message(MSG_TYPE_SEND_MSG, "2");
                    send_message(comfd, response);
                    free(response);
                    {
                        std::lock_guard<std::mutex> lock(coutMutex);
                        std::cout << "Client not found" << std::endl;
                    }
                }
                break;
            }
            default: {
                std::lock_guard<std::mutex> lock(coutMutex);
                std::cout << ERROR << "Unknown message type: " << msg->type << std::endl;
                break;
            }
        }
        free(msg);
    }
    {
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cout << "Client disconnected from " << clientAddr.ip << ": " << clientAddr.port << std::endl;
    }
    {
        std::lock_guard<std::mutex> lock(clientMutex);
        clients.erase(std::remove(clients.begin(), clients.end(), CONN_LIST(comfd, clientAddr, true)), clients.end());
    }
    close_socket(comfd);
}

std::vector<std::thread> threadPool;

int main() {
    struct sigaction action;
    action.sa_handler = signalHandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGINT, &action, nullptr);

    int comfd;
    ADDRESS clientAddr;
    create_socket(sockfd);
    if(sockfd < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return -1;
    }
    int optval = 1;
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        close_socket(sockfd);
        std::cerr << "Error setting socket options" << std::endl;
        return -1;
    }
    int flags = fcntl(sockfd, F_GETFL, 0);
    if(flags < 0) {
        close_socket(sockfd);
        std::cerr << "Error getting socket flags" << std::endl;
        return -1;
    }
    flags |= O_NONBLOCK;
    if(fcntl(sockfd, F_SETFL, flags) < 0) {
        close_socket(sockfd);
        std::cerr << "Error setting socket flags" << std::endl;
        return -1;
    }
    if(Bind(sockfd, SERVER_PORT) < 0) {
        close_socket(sockfd);
        std::cerr << "Error binding socket" << std::endl;
        return -1;
    }

    if(listen(sockfd, 5) < 0) {
        close_socket(sockfd);
        std::cerr << "Error listening on socket" << std::endl;
        return -1;
    }
    std::cout << "Server is listening on port " << SERVER_PORT << std::endl;

    while(true) {
        if(killServer) break;
        if(acceptNewConnection(sockfd, comfd, clientAddr) < 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(USER_SLEEP_TIME));
            continue;
        }
        {
            std::lock_guard<std::mutex> lock(coutMutex);
            std::cout << "New connection from " << clientAddr.ip << ":" << clientAddr.port << std::endl;
        }
        {
            std::lock_guard<std::mutex> lock(clientMutex);
            clients.push_back(CONN_LIST(comfd, clientAddr, true));
        }
        threadPool.emplace_back(handleClient, comfd, clientAddr);
    }
    for(auto& i: threadPool) i.join();
    {
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cout << "Server shutdown" << std::endl;
    }
    return 0;
}
