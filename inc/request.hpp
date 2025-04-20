#pragma once

#include "string.hpp"
#include "config.hpp"

#define BUFFER_SIZE 8000
#define MAX_LINE 1024

class HttpExcept : public exception {
private:
    int statusCode;
    string statusMessage;

public:
    HttpExcept(int code, const string& message) 
        : statusCode(code), statusMessage(message) {}

    int getStatusCode() const {
        return statusCode;
    }
    const char* what() const throw() {
        return statusMessage.c_str();
    }
};

enum ParseState {
    REQUEST_LINE,
    HEAD,
    BODY,
    END_REQUEST
};

class HttpRequest {
public:
    // string key;
    // ssize_t req_size;
    // int bytesRead;
    string buffer;
    ParseState lineLocation;
    string method, uri, host, connection, version;
    string body; // change
    vector<char> bodyChunked;
    map<string, string> headers;
    size_t contentLength;
    servcnf conf;
    routeCnf mtroute;
    bool isPostKeys;
    bool isChunked;
    bool isCGI;
    ofstream bodyFile;

    HttpRequest ();
    HttpRequest(servcnf config) : lineLocation(REQUEST_LINE) , isPostKeys(false) , isChunked(false), isCGI(false) ,conf(config) {};
    // method of reqeust
    bool request(int clientFd);
    void parseRequestLine ();
    void parseHeader();
    void parseBody();
    void HandleUri();
};

void handle_client_read(int clientFd, int epollFd, mpserv& conf, map<int, HttpRequest>& requestStates);

void sendErrorResponse(int fd, int statusCode, const string& message, servcnf& serverConfig);
void modifyState(int epollFd ,int clientFd, uint32_t events);
string getInfoClient(int clientFd);
/* requestParser file */
// void checkBody(const servcnf& server, const HttpRequest& req);
// void checkAllowed(routeCnf route, const string& method, const string& path);
// void getRoute(const servcnf& server, const string& path, string& matched, HttpRequest& req);
// void checkHeaders(const HttpRequest& req);
// void checkURI(const string& path);
// void checkMethod(const string& method);
