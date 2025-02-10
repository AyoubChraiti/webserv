#pragma once

#include <sys/socket.h>
#include <iostream>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <netdb.h> 
#include <arpa/inet.h>
#include <sys/types.h>
#include <cstdlib>

using namespace std;

struct WebServerConfig;

void sysCallFail();
void serverSetup(WebServerConfig serv);

template <typename T>
std::string to_string(T value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}
