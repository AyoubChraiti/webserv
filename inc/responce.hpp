#pragma once

#include "config.hpp"
#include "header.hpp"
#include "request.hpp"

struct RouteResult {
    int statusCode;
    string statusText;
    string responseBody;
    string contentType;
    string redirectLocation;
    bool shouldRDR;
    int resFd;
    string fullPath;
};

class Response 
{
private:
    int statusCode;
    string statusText;
    map<string, string> headers;
    unsigned int contentLength;
    string body;
public:
    Response() : statusCode(200),  statusText("OK") {};
    void buildResponse (servcnf& conf, HttpRequest &reqStates, int clientFd);
};

void handle_client_write(int clientFd, int epollFd, mpserv& conf, map<int, HttpRequest>& reqStates) ;
void closeOrSwitch(int clientFd, int epollFd, HttpRequest& req, map<int, HttpRequest>& requestmp);
void sendRedirect(int clientFd, const string& location, HttpRequest& req);
bool isDirectory(const string& path);
string getContentType(const string& filepath);
string generateAutoIndex(const string& fullPath, const string& uriPath);
RouteResult handleRouting(int fd, HttpRequest& req);
bool fileExists(const string& path);
bool isDirectory(const string& path);
