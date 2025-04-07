#include "../../inc/request.hpp"

const char *HttpRequest::RequestException::what() const throw ()
{
    return errorResponse.c_str();
}

string sendErrorResponse(const char *e, int clientSocket)
{
    string str = e;
    size_t  pos = str.find_last_of("\n");
    string errorMessage = str.substr(0, pos);
    int statusCode = atoi(str.substr(pos).c_str());
    std::string response = 
        "HTTP/1.1 " + std::to_string(statusCode) + " " + errorMessage + "\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n"
        "\r\n"
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head><title>" + std::to_string(statusCode) + " " + errorMessage + "</title></head>\n"
        "<body>\n"
        "<h1>" + errorMessage + "</h1>\n"
        "<p>Error code: " + std::to_string(statusCode) + "</p>\n"
        "</body>\n"
        "</html>";
    
    send(clientSocket, response.c_str(), response.length(), 0);
    return "Client :" + errorMessage;
}

HttpRequest::RequestException::RequestException (string msg , int status) : errorStr(msg) , statusCode(status)
{
    errorResponse += errorStr + '\n' + to_string(statusCode);
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
bool HttpRequest::request(int clientFd, int epollFd, servcnf &reqConfig)
{
    char buff[BUFFER_SIZE];
    ssize_t recvBytes = recv(clientFd, buff, BUFFER_SIZE - 1, 0);
    if (recvBytes == 0)
        return true;
    else if (recvBytes < 0)
        throw RequestException("error in recv", 500); // check after
    if (recvBytes > 0)
    {
        buff[recvBytes] = '\0';
        buffer.append(buff, recvBytes);
        if (lineLocation == REQUEST_LINE)
            parseRequestLine(reqConfig); 
        if (lineLocation == HEAD)
            parseHeader(reqConfig); 
        if (lineLocation == END_REQUEST)
            return true;
    }
    return false;
}

void handleClientRequest(int clientFd, int epollFd, mpserv& conf, map<int, HttpRequest> &reqStates)
{
    string host = SetupClient(clientFd, reqStates);
    servcnf reqConfig = conf.servers[host];
    try 
    {
        if (reqStates[clientFd].request(clientFd, epollFd, reqConfig))
        {
            struct epoll_event ev;
            ev.events = EPOLLOUT;
            ev.data.fd = clientFd;
            if (epoll_ctl(epollFd, EPOLL_CTL_MOD, clientFd, &ev) == -1) 
                return ;
            std::string responseBody = "<html><body><h1>200 OK - Welcome to the Server!</h1></body></html>";
            std::string response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: " + std::to_string(responseBody.length()) + "\r\n"
            "Connection: close\r\n"
            "\r\n" +
            responseBody;
            send(clientFd, response.c_str(), response.length(), 0);
            reqStates.erase(clientFd);
        }
    }
    catch(const std::exception& e)
    {
        struct epoll_event ev;
        ev.events = EPOLLOUT;
        ev.data.fd = clientFd;
        if (epoll_ctl(epollFd, EPOLL_CTL_MOD, clientFd, &ev) == -1) 
            return ;
        reqStates.erase(clientFd);
        cerr << sendErrorResponse(e.what(), clientFd) << endl;
        epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, NULL);
    }
    close(clientFd);
}
