#include "../../inc/request.hpp"

string getInfoClient(int clientFd)
{
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    if (getsockname(clientFd, (struct sockaddr*) &addr, &addrlen))
        sysCallFail();
    char ipStore[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(addr.sin_addr), ipStore, INET_ADDRSTRLEN);
    int port = ntohs(addr.sin_port);
    string host = static_cast <string> (ipStore) + ":" + to_string(port);
    return host;
}

void modifyState(int epollFd ,int clientFd, uint32_t events)
{
    struct epoll_event ev;
    ev.events = events;
    ev.data.fd = clientFd;
    if (epoll_ctl(epollFd, EPOLL_CTL_MOD, clientFd, &ev) == -1) 
        return ;
}

string  HttpRequest::getMethod () const
{
    return method;
}
string HttpRequest::getURI () const
{
    return uri;
}
bool HttpRequest::request(int clientFd, int epollFd, servcnf &reqConfig)
{
    char buff[BUFFER_SIZE];
    ssize_t recvBytes = recv(clientFd, buff, BUFFER_SIZE - 1, 0);
    if (recvBytes == 0)
        return true;
    else if (recvBytes < 0)
        throw RequestException("Internal Server Error", 500);
    if (recvBytes > 0)
    {
        buff[recvBytes] = '\0';
        cout << buff << endl;
        buffer.append(buff, recvBytes);
        if (lineLocation == REQUEST_LINE)
            parseRequestLine(reqConfig); 
        if (lineLocation == HEAD)
            parseHeader(reqConfig);
        if (lineLocation == BODY)
            parseBody(reqConfig);
        if (lineLocation == END_REQUEST)
            return true;
    }
    return false;
}

void handleClientRequest(int clientFd, int epollFd, mpserv& conf, map<int, HttpRequest> &reqStates)
{
    map<int, HttpRequest>::iterator it = reqStates.find(clientFd);
    if (it == reqStates.end())
        reqStates.insert(make_pair(clientFd, HttpRequest()));
    string host = getInfoClient(clientFd);
    servcnf reqConfig = conf.servers[host];
    try 
    {
        if (reqStates[clientFd].request(clientFd, epollFd, reqConfig))
            modifyState(epollFd, clientFd, EPOLLOUT);
    }
    catch(const std::exception& e)
    {
        modifyState(epollFd, clientFd, EPOLLOUT);
        cerr << sendErrorResponse(e.what(), clientFd) << endl;
        reqStates.erase(clientFd);
        epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, NULL); 
        close(clientFd);
    }
}
