#pragma once

#include "config.hpp"

#define BUFFER_SIZE 8192 // <----------------
#define MAX_URI_LENGTH 1024


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
    virtual ~HttpExcept() throw() {}
};

enum ParseState {
    READING_REQUEST_LINE,
    READING_HEADERS,
    READING_BODY,
    COMPLETE
};

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

class HttpRequest {
public:
    string key;
    string buffer;
    ParseState state;
    string method, uri, host, connection, version, querystring;
    map<string, string> headers;
    size_t contentLength;
    int bytesRead;
    servcnf conf;
    routeCnf mtroute;
    fstream* bodyFile;
    RouteResult routeResult;
    bool sendingFile;
    size_t bytesSentSoFar;
    bool headerSent;
    int clientFd;
    string outputCGI;
    bool isPostKeys;
    bool isChunked;
    bool isCGI;
    size_t remaining;
    string Boundary ; 
    bool startBoundFlag;

    HttpRequest(servcnf config);
    ~HttpRequest();

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

void sendErrorResponse(int fd, int statusCode, const string& message, servcnf& serverConfig);
RouteResult handleRouting(int fd, HttpRequest& req);

// requestParser file
void handle_client_read(int clientFd, int epollFd, mpserv& conf, map<int, HttpRequest> &reqStates, map<int , HttpRequest *> &pipes_map);
void modifyState(int epollFd ,int clientFd, uint32_t events);
string getInfoClient(int clientFd);
string getFileName(string buff);
size_t hexToInt (const string &str);
size_t StringStream(const string &string);
bool isValidContentLength (const string &value);
bool isValidHostHeader(const string& host) ;
void writebody(fstream &bodyFile , string &buffer);


// CgiHandler headers
int HandleCGI (int epollFd ,int clientFd, map<int, HttpRequest> &reqStates, map<int, HttpRequest *> &pipes_map);
void handle_cgi_write(int writeFd, int epollFd,map<int, HttpRequest *> &pipes_map);
void handle_cgi_read(int readFd, int epollFd, HttpRequest *reqStates);
void parseCGIoutput (string &outputCGI);


