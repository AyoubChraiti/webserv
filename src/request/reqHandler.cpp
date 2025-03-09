#include "../../inc/request.hpp"

const char *HttpRequest::RequestException::what() const throw ()
{
    return errorString.c_str();
}

void SetupClient(int clientFd, map<int, HttpRequest> &reqStates)
{
    map<int, HttpRequest>::iterator it = reqStates.find(clientFd);
    if (it == reqStates.end())
        reqStates.insert(make_pair(clientFd, HttpRequest()));
}
void HttpRequest::request(int clientFd, int epollFd, mpserv& conf)
{
    char buff[BUFFER_SIZE];
    ssize_t recvBytes = recv (clientFd, buff, BUFFER_SIZE , 0);
    if (recvBytes > 0)
    {
        buffer.append(buff);
        if (!lineLocation)
        {
            size_t index = 0;
            vector <string> hold(3);
            while ((index = buffer.find(' ')) != string::npos)
            {
                hold.push_back(buffer.substr(0, index));
                buffer.erase(0, index);
            }
            method = hold[0];
            uri = hold[1];
            HttpVersion = hold[2];
        }
        cout << method << "-" << uri << "-" << HttpVersion << endl;
        exit(1);
        if (buffer.find("\r\n\r\n") == string::npos)
            return ;
    }
    struct epoll_event structEvent;
    structEvent.events = EPOLLOUT;
    structEvent.data.fd = clientFd;
    epoll_ctl (epollFd, EPOLL_CTL_MOD, clientFd, &structEvent);
    send(clientFd, RESPONSE, strlen(RESPONSE), 0);
}
void SendErrorRespone()
{
}

void handleClientRequest(int clientFd, int epollFd, mpserv& conf, map<int, HttpRequest> &reqStates)
{
    SetupClient(clientFd, reqStates);
    try
    {
        reqStates[clientFd].request(clientFd, epollFd, conf);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        SendErrorRespone();
    }
}








   // stringstream ss(buff);
    // ss >> req.method >> req.uri >> req.HttpVersion;
    // if (req.method.empty() || req.uri.empty() || req.HttpVersion.empty())
        // throw Request::RequestException("Bad Request", 400);
    // if (req.method != "GET" && req.method != "POST" && req.method != "DELETE")
        // throw Request::RequestException("Not Implemented", 501);
    // if (req.HttpVersion != "HTTP/1.1")
        // throw Request::RequestException("HTTP Version Not Supported", 505);
