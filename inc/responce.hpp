#pragma once

#include "config.hpp"
#include "header.hpp"
#include "request.hpp"

struct RouteResult {
    int statusCode;
    string statusText;
    string responseBody;
    string contentType;
    bool shouldRedirect;
    string redirectLocation;
};

void handle_client_write(int clientFd, int epollFd, mpserv& conf, map<int, HttpRequest>& requestmp);
void closeOrSwitch(int clientFd, int epollFd, HttpRequest& req, map<int, HttpRequest>& requestmp);
void sendRedirect(int clientFd, const string& location, HttpRequest& req);
bool isDirectory(const string& path);
string getContentType(const string& filepath);
string generateAutoIndex(const string& fullPath, const string& uriPath);
RouteResult handleRouting(HttpRequest& req);
