#pragma once

#include "config.hpp"
#include "header.hpp"
#include "request.hpp"



void handle_client_write(int clientFd, int epollFd, map<int, Http *>& requestmp, map<int, Http *> &pipes_map, map<int, time_t>& timer);
void sendRedirect(int epollFd, int fd, const string& location, Http* req, map<int, Http*>& requestmp);
bool isDirectory(const string& path);
string getContentType(const string& filepath);
string generateAutoIndex(const string& fullPath, const string& uriPath);
bool fileExists(const string& path);
bool isDirectory(const string& path);

void sendHeaders(int epollFd, int clientFd, RouteResult& routeResult, Http* req, map<int, Http*>& requestmp);
void deleteMethod(int epollFd, int clientFd, Http* req, map<int, Http*>& requestmp);
int getMethode(int clientFd, Http* req, map<int, Http*>& requestmp, int epollFd);
size_t getContentLength(const string& path);