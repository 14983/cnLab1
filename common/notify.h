#ifndef NOTIFY_H
#define NOTIFY_H

#include <string>
#include <cstdlib>

void notify(const std::string &message) {
    std::string command = "notify-send \"" + message + "\"";
    system(command.c_str());
}

#endif