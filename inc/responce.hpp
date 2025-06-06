#pragma once

#include "config.hpp"
#include "header.hpp"
#include "request.hpp"



void handle_client_write(int clientFd, int epollFd, map<int, Http *>& requestmp, map<int, Http *> &pipes_map, map<int, time_t>& timer);
void sendRedirect(int fd, const string& location, Http* req);
bool isDirectory(const string& path);
string getContentType(const string& filepath);
string generateAutoIndex(const string& fullPath, const string& uriPath);
bool fileExists(const string& path);
bool isDirectory(const string& path);

void sendHeaders(int clientFd, RouteResult& routeResult, Http* req);
void deleteMethod(int clientFd, Http* req);
int getMethode(int clientFd, Http* req);
size_t getContentLength(const string& path);