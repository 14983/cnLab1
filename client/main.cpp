#include "../common/common.h"
#include "../common/notify.h"

int sockfd = -1;
bool message_handler_running = false, notify_sender_running = true;
std::thread message_handler_thread, notify_sender_thread;

class msgQueue {
public:
    void push(MSG *msg) {
        std::lock_guard<std::mutex> lock(_mtx);
        _queue.push(msg);
        _cv.notify_one();
    }
    MSG *pop() {
        std::unique_lock<std::mutex> lock(_mtx);
        _cv.wait(lock, [this]{ return!_queue.empty(); });
        MSG *msg = _queue.front();
        _queue.pop();
        return msg;
    }
    bool empty() {
        std::lock_guard<std::mutex> lock(_mtx);
        return _queue.empty();
    }
private:
    std::queue<MSG *> _queue;
    std::mutex _mtx;
    std::condition_variable _cv;
};
msgQueue messages;

void blocked_send(const MSG *msg){
    for (int i = 0; i < TIMEOUT/USER_SLEEP_TIME; i++){
        int ret = send_message(sockfd, msg);
        if (ret <= 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(USER_SLEEP_TIME));
        } else {
            return;
        }
    }
    std::cout << ERROR << "blocked_send timeout" << std::endl;
}

int connect_to_server(int &sockfd, const std::string &address){
    struct sockaddr_in servAddr;
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(SERVER_PORT);
    servAddr.sin_addr.s_addr = inet_addr(address.c_str());
    bzero(&(servAddr.sin_zero), 8);
    int connect_status = connect(sockfd, (struct sockaddr *) &servAddr, sizeof(servAddr));
    if (connect_status < 0 && errno != EINPROGRESS && errno != EAGAIN) {
        return -1;
    }
    pollfd pfd;
    pfd.fd = sockfd;
    pfd.events = POLLOUT;
    int poll_status = poll(&pfd, 1, TIMEOUT); // wait for 1 second for connection to be established
    if (poll_status < 0) {
        return -1;
    }
    if (poll_status == 0) {
        // timeout
        return -1;
    }
    socklen_t len = sizeof(connect_status);
    getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &connect_status, &len);
    if (connect_status != 0) {
        return -1;
    }
    return 0;
}

int message_handler(void) { // this is called by a new thread
    std::cout << THREAD << "started message_handler thread" << std::endl;
    MSG *msg = NULL;
    int ret;
    while (message_handler_running) {
        ret = get_message(sockfd, msg);
        if (ret <= 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(USER_SLEEP_TIME));
            continue; // no message available
        }
        messages.push(msg);
    }
    std::cout << THREAD << "message_handler thread stopped" << std::endl;
    return 0;
}


void notify_sender(void){
    std::cout << THREAD << "started notify_sender thread" << std::endl;
    while (notify_sender_running) {
        if (messages.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(USER_SLEEP_TIME));
            continue; // no message available
        }
        MSG *msg = messages.pop(); // block until a message is available
        assert(msg != NULL);
        if (msg->type == MSG_TYPE_GET_NAME){
            notify("Get hostname: " + std::string(msg->data));
        } else if (msg->type == MSG_TYPE_GET_TIME){
            notify("Get time: " + std::string(msg->data));
        } else if (msg->type == MSG_TYPE_GET_LIST){
            std::string notify_str = "Get client list: ";
            char *ptr = (char *)msg->data;
            int count = *(int *)ptr;
            ptr += sizeof(int);
            while (count--){
                int comfd = *(int *)ptr;
                ptr += sizeof(int);
                int ip = *(int *)ptr;
                ptr += sizeof(int);
                int port = *(int *)ptr;
                ptr += sizeof(int);
                notify_str += std::to_string(comfd) + " " + inet_ntoa(*(in_addr *)&ip) + ":" + std::to_string(port) + ", ";
            }
            notify(notify_str);
        } else if (msg->type == MSG_TYPE_SEND_MSG){
            switch (msg->data[0]) {
                case '0':
                    notify("Send message to client successfully");
                    break;
                case '1':
                    notify("Send message to client failed");
                    break;
                case '2':
                    notify("No such client");
                    break;
                default:
                    notify("UNKNOWN ERROR");
                    break;
            }
        } else if (msg->type == MSG_TYPE_RECV_MSG){
            std::string notify_str = "\'" + std::string((char *)msg->data + 12) + "\' from " + std::string(inet_ntoa(*(in_addr *)((char *)msg->data+4))) + ":" + std::to_string(*(int *)((char *)msg->data + 8));
            notify(notify_str);
        } else {
            notify("unsupported message type: " + std::to_string(msg->type));
        }
        free(msg);
    }
    std::cout << THREAD << "notify_sender thread stopped" << std::endl;
}

int main(int argc, char *argv[]) {
    // start notify sender thread, continue to work until the program quits
    notify_sender_thread = std::thread(notify_sender);

    static const std::string usage = "Usage:\n\t1 - Connect to server\n\t2 - Disconnect from server\n\t3 - Get time\n\t4 - Get hostname\n\t5 - Get client list\n\t6 - Send message to client\n\t7 - Quit\n";
    std::string command;
    while (true){
        std::cout << usage << "Enter command: ";
        std::cin >> command;
        if (command == "1") {
            // check if already connected, message_handler_running is set to true if connected
            if (message_handler_running) {
                std::cout << WARNING << "already connected" << std::endl;
                continue;
            }
            // create socket and connect to server
            create_socket(sockfd);
            if (sockfd < 0) {
                std::cout << ERROR << "creating socket" << std::endl;
                continue;
            }
            std::cout << INFO << "created socket" << std::endl;
            std::cout << "Enter server address: ";
            std::string address;
            std::cin >> address;
            if (connect_to_server(sockfd, address) < 0) {
                std::cout << ERROR << "connecting to server" << std::endl;
                continue;
            }
            std::cout << INFO << "connected to server" << std::endl;
            // start message handler thread
            message_handler_running = true;
            message_handler_thread = std::thread(message_handler);
        } else if (command == "2" || command == "7") {
            // check if connected
            if (!message_handler_running) {
                if (command == "7") {
                    notify_sender_running = false;
                    break;
                }
                std::cout << ERROR << "not connected" << std::endl;
                continue;
            }
            // if connected, send MSG_TYPE_END_CONN to server
            MSG *msg = create_message(REQ_TYPE_END_CONN);
            blocked_send(msg);
            free(msg);
            std::cout << INFO << "sent MSG_TYPE_END_CONN" << std::endl;
            // wait for message handler thread to join
            message_handler_running = false;
            message_handler_thread.join();
            std::cout << THREAD << "joined message thread" << std::endl;
            // close socket
            close_socket(sockfd);
            std::cout << INFO << "closed socket" << std::endl;
            // if command is 7, tell main thread to quit
            if (command == "7") {
                notify_sender_running = false;
                break;
            }
        } else if (command == "3") {
            // check if connected
            if (!message_handler_running) {
                std::cout << ERROR << "not connected" << std::endl;
                continue;
            }
            // if connected, send REQ_TYPE_GET_TIME to server
            MSG *msg = create_message(REQ_TYPE_GET_TIME);
            blocked_send(msg);
            free(msg);
            std::cout << INFO << "sent REQ_TYPE_GET_TIME" << std::endl;
        } else if (command == "4") {
            // check if connected
            if (!message_handler_running) {
                std::cout << ERROR << "not connected" << std::endl;
                continue;
            }
            // if connected, send MSG_TYPE_GET_NAME to server
            MSG *msg = create_message(REQ_TYPE_GET_NAME);
            blocked_send(msg);
            free(msg);
            std::cout << INFO << "sent REQ_TYPE_GET_NAME" << std::endl;
        } else if (command == "5") {
            // check if connected
            if (!message_handler_running) {
                std::cout << ERROR << "not connected" << std::endl;
                continue;
            }
            // if connected, send MSG_TYPE_GET_LIST to server
            MSG *msg = create_message(REQ_TYPE_GET_LIST);
            blocked_send(msg);
            free(msg);
            std::cout << INFO << "sent REQ_TYPE_GET_LIST" << std::endl;
        } else if (command == "6") {
            // check if connected
            if (!message_handler_running) {
                std::cout << ERROR << "not connected" << std::endl;
                continue;
            }
            // if connected, send MSG_TYPE_SEND_MSG to server
            std::cout << "Enter client ID: ";
            int client_id;
            std::cin >> client_id;
            std::cout << "Enter message, type `.` to end" << std::endl;
            std::string message, tmp;
            std::getline(std::cin, tmp);
            while (true) {
                std::cout << "\033[32m> \033[0m";
                std::getline(std::cin, tmp);
                if (tmp == ".") {
                    break;
                }
                message += tmp;
            }
            char *buffer = (char *) malloc(sizeof(client_id) + message.length() + 1);
            memcpy(buffer, &client_id, sizeof(client_id));
            memcpy(buffer + sizeof(client_id), message.c_str(), message.length());
            buffer[sizeof(client_id) + message.length()] = '\0';
            MSG *msg = create_message(REQ_TYPE_SEND_MSG, sizeof(client_id) + message.length() + 1, buffer);
            blocked_send(msg);
            free(buffer);
            free(msg);
            std::cout << INFO << "sent REQ_TYPE_SEND_MSG to client " << client_id << std::endl;
        }
    }
    // wait for notify_sender thread to join
    notify_sender_running = false;
    notify_sender_thread.join();
    std::cout << THREAD << "joined notify thread" << std::endl;
    // quit
    std::cout << THREAD << "quitting main thread" << std::endl;
}