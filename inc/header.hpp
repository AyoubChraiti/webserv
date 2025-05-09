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
#include <sys/stat.h>
#include <signal.h>
#include <sys/types.h>
#include <dirent.h>
#include <iomanip>
#include <algorithm>
#include <sys/wait.h>

using namespace std;

extern bool shutServer;

struct mpserv;
class HttpRequest;

/* error handling */
void sysCallFail();

/* server setup */
void webserver(mpserv &conf);

/* c++11 functions */
template <typename T>
string to_string(T value) {
    ostringstream oss;
    oss << value;
    return oss.str();
}

void ctrl_C();

string getIp(string hostname);
string trim(const string& str);
void add_fds_to_epoll(int epollFd, int fd, uint32_t events);
void sigchld_handler(int);