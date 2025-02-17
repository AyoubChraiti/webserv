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
#include <sys/epoll.h>
#include <algorithm>

using namespace std;

struct mpserv;

void sysCallFail();
void serverSetup(mpserv &conf, vector<int> &servrs);
void webserver(mpserv &conf);
void request(int fd, mpserv &conf);
void testConfigParser(const string &filePath);

template <typename T>
string to_string(T value) {
    ostringstream oss;
    oss << value;
    return oss.str();
}
