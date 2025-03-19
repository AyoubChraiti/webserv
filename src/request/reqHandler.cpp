#include "../../inc/request.hpp"

const char *HttpRequest::RequestException::what() const throw ()
{
    return errorResponse.c_str();
}
void HttpRequest::RequestException::createErrorResponse()
{
    errorResponse = "HTTP1.1 " + to_string (statusCode) + " " + errorStr + "\r\n";
    errorResponse += "Content-Type: text/plain\r\n";
    errorResponse += "Connection: close\r\n";
    errorResponse += "\r\n";
    errorResponse += errorStr;
}
HttpRequest::RequestException::RequestException (string msg , int status) : errorStr(msg) , statusCode(status)
{
    createErrorResponse();
    switch (statusCode)
    {
        case 400:
            errorStr =  "Bad Request"; break;
        case 414:
            errorStr = "URI TOO Long"; break;
        case 501:
            errorStr = "Not Implemented"; break;
        case 505:
            errorStr = "HTTP Version Not Supported"; break;
        case 405:
            errorStr = "Method Not Allowed"; break;
        default:
            errorStr  = "Undefined Error" ; break;
    }
}
void HttpRequest::request(int clientFd, int epollFd, servcnf &reqConfig)
{
    char buff[BUFFER_SIZE];
    ssize_t recvBytes = recv (clientFd, buff, BUFFER_SIZE , 0);
    if (recvBytes > 0)
    {
        buff[BUFFER_SIZE - 1] = '\0';
        buffer.append(buff, recvBytes);
        switch (lineLocation)
        {
            case REQUEST_LINE:
                parseRequestLine(reqConfig);
            case HEAD : 
                parseHeader(reqConfig);
            case END_REQUEST:
                cout << "END OF REQUEST"  << endl; break;
        }
    }
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
string sendErrorResponse(const char *e, int clientFd)
{
    string str = e;
    size_t  pos = str.find_last_of("\r\n");
    send(clientFd , str.substr(0, pos + 1).c_str(),  pos, 0);
    return str.substr(pos);
}
void handleClientRequest(int clientFd, int epollFd, mpserv& conf, map<int, HttpRequest> &reqStates)
{
    string host = SetupClient(clientFd, reqStates);
    servcnf reqConfig = conf.servers[host];
    try 
    {
        reqStates[clientFd].request(clientFd, epollFd, reqConfig);
        close(clientFd);
    }
    catch(const std::exception& e)
    {
        cerr << sendErrorResponse(e.what(), clientFd) << endl;
    }
    close(clientFd);
}
