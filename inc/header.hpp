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

using namespace std;

struct mpserv;
class HttpRequest;

void sysCallFail();
void serverSetup(mpserv &conf, vector<int> &servrs);
void webserver(mpserv &conf);
void testConfigParser(const string &filePath);
std::string trim(const std::string& str);

void sendErrorResponse(int fd, int statusCode, const string& message);

bool isValidDirectory(const string &path);
bool isValidFile(const string &path);


void handle_client_write(int clientFd, int epollFd, mpserv& conf, std::map<int, HttpRequest>& requestStates);

template <typename T>
string to_string(T value) {
    ostringstream oss;
    oss << value;
    return oss.str();
}


string getIp(string hostname);