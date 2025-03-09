#pragma once

#include "header.hpp"
#include "config.hpp"
#define BUFFER_SIZE 50
#define RESPONSE "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 13\r\n\r\nHello, world!"


class HttpRequest
{
    private:
    string buffer;
    int lineLocation;
    string method, uri, HttpVersion;
    map<string, string> headers;

    public:
    HttpRequest() : lineLocation(0) {};
    void request(int clientFd, int epollFd, mpserv& conf);
    // Exception
    class RequestException : public exception
    {
        private:
            string errorString;
            int statusCode;
        public:
            RequestException (string msg, int  status) : errorString(msg), statusCode(status) {}; 
            const char *what() const throw();
    };
};

void handleClientRequest(int clientFd, int epollFd, mpserv& conf, map<int, HttpRequest> &reqStates);
/*
    "
    GET / HTTP/1.1\r\n
    Host: localhost\r\n
    connection: keep-alive\r\n
    \r\n
    \0balasbdjasdbsajk"
*/
//starter-line;
//headers


