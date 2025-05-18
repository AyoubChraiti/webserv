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

enum CGIState {
    HEADERS_CGI,
    BODY_CGI,
    COMPLETE_CGI
};
struct RouteResult {
    int statusCode;
    string statusText;
    string responseBody;
    string contentType;
    string redirectLocation;
    bool shouldRDR;
    bool autoindex;
    ifstream* fileStream;

    RouteResult() :statusCode(200), statusText("OK"), responseBody(""), contentType("text/plain"),
        redirectLocation(""), shouldRDR(false), autoindex(false), fileStream(NULL) {}
};

class Http {
public:
    string key;
    string buffer;
    ParseState state;
    string method, uri, host, connection, version, querystring;
    map<string, string> headers;
    RouteResult routeResult;
    size_t contentLength;
    servcnf conf;
    routeCnf mtroute;
    fstream bodyFile;
    size_t bytesSentSoFar;
    string outputCGI;
    size_t remaining;
    string Boundary ; 
    string fullPath;
    string _extensionCGI;
    CGIState stateCGI;
    int clientFd;
    int stdinFd;
    int stdoutFd;
    int bytesRead;
    bool sendingFile;
    bool startBoundFlag;
    bool headerSent;
    bool isPostKeys;
    bool isChunked;
    bool isCGI;
    bool hasBody;
    int cgiPid;

    Http(servcnf config);
    ~Http();

    bool request(int clientFd);
    void parseRequestLine ();
    void HandleHeaders();
    void ParseHeaders();
    void parseBody();
    void HandleUri();
    void HandleChunkedBody();
    void HandleBoundary() ;
    void openFile (string name);
    void checkIsCGI();
    void checkPost();
};

void sendErrorResponse(int fd, int statusCode, const string& message, servcnf& serverConfig);
RouteResult handleRouting(Http* req);

// requestParser file
void handle_client_read(int clientFd, int epollFd, mpserv& conf, map<int, Http *> &req, map<int, Http *> &pipes_map, map<int, time_t>& timer);
void modifyState(int epollFd ,int clientFd, uint32_t events);
string getInfoClient(int clientFd);
string getFileName(string buff);
size_t hexToInt (const string &str);
size_t StringStream(const string &string);
bool isValidContentLength (const string &value);
bool isValidHostHeader(const string& host) ;
void writebody(fstream &bodyFile , string &buffer);
void closeOrSwitch(int clientFd, int epollFd, Http* req, map<int, Http *>& requestmp);
void sendPostResponse(int clientFd, int epollFd, Http* req, map<int, Http *> &reqStates);


// CgiHandler headers

int HandleCGI (int epollFd, int clientFd, map<int, Http *> &reqStates, map<int, Http *> &pipes_map, map<int , time_t> &timer);
void handle_cgi_write(int writeFd, int epollFd, map<int, Http *> &pipes_map, map<int, time_t> timer);
void handle_cgi_read(int epollFd, int readFd, Http *reqStates, map<int, Http *> &pipes_map);
bool CGImonitor(int epollFd, map<int, Http *> &request, map<int, Http *> &pipes_map, map<int, time_t>& timer) ;
string strUpper(string str);

