#pragma once

#include "config.hpp"
#include "header.hpp"
#include "request.hpp"



void handle_client_write(int clientFd, int epollFd, map<int, HttpRequest *>& requestmp);
void closeOrSwitch(int clientFd, int epollFd, HttpRequest* req, map<int, HttpRequest *>& requestmp);
void sendRedirect(int clientFd, const string& location, HttpRequest* req);
bool isDirectory(const string& path);
string getContentType(const string& filepath);
string generateAutoIndex(const string& fullPath, const string& uriPath);
bool fileExists(const string& path);
bool isDirectory(const string& path);

void sendHeaders(int clientFd, RouteResult& routeResult, HttpRequest* req);
void deleteMethod(int clientFd, HttpRequest* req);
int getMethode(int clientFd, HttpRequest* req);
size_t getContentLength(const string& path);
// void RDR(int fd, int epollFd, HttpRequest& req, map<int, HttpRequest>& requestmp);