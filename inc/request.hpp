#pragma once

#include "header.hpp"
#include "config.hpp"
#define BUFFER_SIZE 4000
#define RESPONSE "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 13\r\n\r\nHello, world!"


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
    
    public:
    
    HttpRequest() : lineLocation(REQUEST_LINE) {};
    void request(int clientFd, int epollFd, servcnf &reqConfig);
    void parseRequestLine (servcnf &reqConfig);
    void parseHeader(servcnf &reqConfig);
    // Exception
    class RequestException : public exception
    {
        private:
            string errorResponse;
            string errorStr;
            int statusCode;
        public:
            RequestException (string msg, int  status) ;
            void createErrorResponse();
            const char *what() const throw();
    };
};

void handleClientRequest(int clientFd, int epollFd, mpserv& conf, map<int, HttpRequest> &reqStates);

/*
    "
    GET /kjhh HTTP/1.1 sdsdf sdfsf\r\n
    Host: localhost\r\n
    connection: keep-alive\r\n
    \r\n
    \0balasbdjasdbsajk"
*/
//starter-line;
//headers


