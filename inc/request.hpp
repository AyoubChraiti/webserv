#pragma once

#include "string.hpp"
#include "config.hpp"

#define BUFFER_SIZE 8192
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
    READING_REQUEST_LINE,
    READING_HEADERS,
    READING_BODY,
    COMPLETE
};

class HttpRequest {
public:
    string key;
    string buffer;
    ParseState state;
    ssize_t req_size;
    string method, path, host, connection, version;
    vector<char> body; // Used to store binary data safely
    map<string, string> headers;
    size_t contentLength;
    int bytesRead;
    servcnf conf;

    HttpRequest() : state(READING_REQUEST_LINE), contentLength(0), bytesRead(0) {};
    string get(const string& key, const string& defaultValue) const;
    bool parseRequestLineByLine(int fd, servcnf& conf);
    void initFromHeader();

    int Parser(const char* data, size_t length);
    void firstLineParser(const string& line);
    void HeadersParsing(const string& line);
    void bodyPart(const char* data, size_t length);
};

int request(int fd, mpserv &conf, int epollFd, map<int, HttpRequest>& requestStates);
void handle_client_read(int clientFd, int epollFd, mpserv& conf, map<int, HttpRequest>& requestStates);
void sendErrorResponse(int fd, int statusCode, const string& message, servcnf& serverConfig);

