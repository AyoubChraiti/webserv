#pragma once

#include "header.hpp"
#include "config.hpp"
#define BUFFER_SIZE 1024


enum {
    REQUEST_LINE,
    HEAD,
    BODY,
    END_REQUEST
};
class HttpRequest
{
    private:
    string buffer;
    int lineLocation;
    string method, uri, HttpVersion;
    map <string, string> headers;
    bool isPostKeys;
    string body;
    size_t sizeBody;
    bool isChunked;
    public:
    
    HttpRequest() : lineLocation(REQUEST_LINE) , isPostKeys(false) , isChunked(false) {};
    bool request(int clientFd, int epollFd, servcnf &reqConfig);
    void parseRequestLine (servcnf &reqConfig);
    void parseHeader(servcnf &reqConfig);
    void parseBody(servcnf &reqConfig);
    string getMethod () const;
    string getURI() const;

    // Exception
    class RequestException : public exception
    {
        private:
            string errorResponse;
            string errorStr;
            int statusCode;
        public:
            RequestException (string msg, int  status) ;
            const char *what() const throw();
    };
};
size_t StringStream(string string);
string getInfoClient(int clientFd);
void handleClientRequest(int clientFd, int epollFd, mpserv& conf, map<int, HttpRequest> &reqStates);
void handle_client_write(int clientFd, int epollFd, mpserv& conf, map<int, HttpRequest>& reqStates) ;


