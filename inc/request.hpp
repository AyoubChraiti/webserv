#pragma once

#include "header.hpp"
#include "config.hpp"

#define BUFFER_SIZE 1024

class HttpRequest;

static map<int, HttpRequest> requestStates;

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

class HttpRequest {
public:
    string buffer;
    ParseState state;
    ssize_t req_size;
    string method, path, host, connection, body, version;
    map<string, string> headers;
    size_t contentLength;
    int bytesRead;

    // --methodes-- //

    HttpRequest() : state(READING_REQUEST_LINE), contentLength(0), bytesRead(0) {};
    string getRequest() const; // wont need this ig
    string get(const string& key, const string& defaultValue) const;
    bool parseRequestLineByLine(int fd);

    void initFromHeader();
};


// void parseChecking(const servcnf& server, const HttpRequest& req);