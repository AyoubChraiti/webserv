#pragma once

#include "string.hpp"
#include "config.hpp"

#define BUFFER_SIZE 8000
#define MAX_LINE 1024
#define MAX_URI_LENGTH 2048


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
    string method, uri, host, connection, version, querystring;
    map<string, string> headers;
    size_t contentLength;
    servcnf conf;
    routeCnf mtroute;
    bool isPostKeys;
    bool isChunked;
    bool isCGI;
    fstream bodyFile;

    size_t remaining;
    string Boundary ; 
    bool startBoundFlag;
    int clientFd;

    string outputCGI;
    HttpRequest();
    HttpRequest(servcnf config);
    // method of reqeust
    bool request(int clientFd);
    void parseRequestLine ();
    void HandleHeaders();
    void ParseHeaders();
    void parseBody();
    void HandleUri();
    void HandleChunkedBody();
    void HandleBoundary() ;
    bool openFile (string filename);
};

void handle_client_read(int clientFd, int epollFd, mpserv& conf, map<int, HttpRequest> &reqStates, map<int , HttpRequest *> &pipes_map);
void sendErrorResponse(int fd, int statusCode, const string& message, servcnf& serverConfig);
void modifyState(int epollFd ,int clientFd, uint32_t events);
string getInfoClient(int clientFd);
string getFileName(string buff);
size_t hexToInt (const string &str);
size_t StringStream(const string &string);
bool isValidContentLength (const string &value);
bool isValidHostHeader(const string& host) ;
void writebody(fstream &bodyFile , string &buffer);

int HandleCGI (int epollFd ,int clientFd, map<int, HttpRequest> &reqStates, map<int, HttpRequest *> &pipes_map);
void handle_cgi_write(int writeFd, int epollFd,map<int, HttpRequest *> &pipes_map);
void handle_cgi_read(int readFd, int epollFd, HttpRequest *reqStates);
// int HandleCGI (int epollFd ,int clientFd, map<int, HttpRequest> &reqStates, map<int, HttpRequest *> &pipes_map);
/* requestParser file */
// void checkBody(const servcnf& server, const HttpRequest& req);
// void checkAllowed(routeCnf route, const string& method, const string& path);
// void getRoute(const servcnf& server, const string& path, string& matched, HttpRequest& req);
// void checkHeaders(const HttpRequest& req);
// void checkURI(const string& path);
// void checkMethod(const string& method);
