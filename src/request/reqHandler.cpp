#include "../../inc/request.hpp"

const char *HttpRequest::RequestException::what() const throw ()
{
    return errorString.c_str();
}
void printmap()
{
    
}
void HttpRequest::request(int clientFd, int epollFd, servcnf &reqConfig)
{
    char buff[BUFFER_SIZE];
    ssize_t recvBytes = recv (clientFd, buff, BUFFER_SIZE , 0);
    if (recvBytes > 0)
    {
        buff[BUFFER_SIZE - 1] = '\0';
        buffer.append(buff, recvBytes);
        if (lineLocation == REQUEST_LINE)
            parseRequestLine(reqConfig);
        if (lineLocation == HEAD)
            parseHeader(reqConfig);
        if (buffer.find("\r\n\r\n") != string::npos)
        {
            cout << "End of Request !" << endl;
            return ;
        }
    }
    exit(1);
    cout << recvBytes << endl;
    struct epoll_event structEvent;
    structEvent.events = EPOLLOUT;
    structEvent.data.fd = clientFd;
    epoll_ctl (epollFd, EPOLL_CTL_MOD, clientFd, &structEvent);
    send(clientFd, RESPONSE, strlen(RESPONSE), 0);
}



string SetupClient(int clientFd, map<int, HttpRequest> &reqStates)
{
    map<int, HttpRequest>::iterator it = reqStates.find(clientFd);
    if (it == reqStates.end())
        reqStates.insert(make_pair(clientFd, HttpRequest()));
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
// void SendErrorRespone()
// {
//     // struct epoll_event structEvent;
//     // structEvent.events = EPOLLOUT;
//     // structEvent.data.fd = clientFd;
//     // epoll_ctl (epollFd, EPOLL_CTL_MOD, clientFd, &structEvent);
// }

void handleClientRequest(int clientFd, int epollFd, mpserv& conf, map<int, HttpRequest> &reqStates)
{
    string host = SetupClient(clientFd, reqStates);
    servcnf reqConfig = conf.servers[host];
    try 
    {
        reqStates[clientFd].request(clientFd, epollFd, reqConfig);
    }
    catch(const std::exception& e)
     {
        std::cerr << e.what() << '\n';
        // SendErrorRespone();
    }
}
