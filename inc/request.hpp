#pragma once

#include "header.hpp"
#include "config.hpp"

#define BUFFER_SIZE 1024

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
    READING_REQUEST_LINE,
    READING_HEADERS,
    READING_BODY,
    COMPLETE
};

struct RequestParser {
    ParseState state;
    string buffer;
    string method;
    string path;
    string version;
    string body;
    map<string, string> headers;
    int contentLength;
    int bytesRead;

    RequestParser() : state(READING_REQUEST_LINE), contentLength(0), bytesRead(0) {}
};

class HttpRequest {
public:
    char buffer[BUFFER_SIZE];
    ssize_t req_size;
    string method, path, host, connection, body, version;
    map<string, string> headers;

    HttpRequest(int fd, RequestParser& parser);
    string getRequest() const;
    void parseRequest(const string& rawRequest);
    string get(const string& key, const string& defaultValue) const;
};



bool parseRequestLineByLine(int fd, RequestParser& parser);

void parseChecking(const servcnf& server, const HttpRequest& req);