// #include "../../inc/request.hpp"

// void parseChecking(const servcnf& server, HttpRequest& req, int fd) {
//     checkMethod(req.method);
//     checkURI(req.uri);
//     checkHeaders(req);

//     string routeName;
//     getRoute(server, req.uri, routeName, req);

//     checkAllowed(req.mtroute, req.method, routeName);
//     checkBody(server, req);
// }

// string HttpRequest::get(const string& key, const string& defaultValue) const {
//     map<string, string>::const_iterator it = headers.find(key);
//     return (it != headers.end()) ? it->second : defaultValue;
// }

// void HttpRequest::initFromHeader() {
//     host = headers["Host"];
//     size_t pos = host.find(":");
//     string ip = host.substr(0, pos) + ":" + host.substr(pos + 1);;
//     host = ip;
//     connection = get("Connection", "close");
// }

// map<int, HttpRequest>::iterator getReqFrmMap(int fd, map<int, HttpRequest>& requestmp) {
//     map<int, HttpRequest>::iterator it = requestmp.find(fd);
//     if (it == requestmp.end()) {
//         requestmp.insert(pair<int, HttpRequest>(fd, HttpRequest()));
//         it = requestmp.find(fd);
//     }
//     return it;
// }

// string CheckServer(int fd) {
//     struct sockaddr_in addr;
//     socklen_t addrlen = sizeof(addr);
    
//     if (getsockname(fd, (struct sockaddr*)&addr, &addrlen) == -1)
//         sysCallFail();
    
//     char ip_str[INET_ADDRSTRLEN];
//     inet_ntop(AF_INET, &(addr.sin_addr), ip_str, INET_ADDRSTRLEN);
//     int port = ntohs(addr.sin_port);
//     string result = string(ip_str) + ":" + to_string(port);
//     return result;
// }

// int request(int fd, mpserv& conf, int epollFd, map<int, HttpRequest>& requestmp) {
//     map<int, HttpRequest>::iterator it = getReqFrmMap(fd, requestmp);

//     HttpRequest& req = it->second;
//     string sockHost = CheckServer(fd);
//     if (conf.servers.find(sockHost) == conf.servers.end()) {
//         throw HttpExcept(400, "No server for this socket");
//     }
//     req.conf = conf.servers[sockHost];
//     req.key = sockHost;

//     try {
//         if (req.parseRequestLineByLine(fd, req.conf)) {
//             req.initFromHeader();
//             parseChecking(req.conf, req, fd);
//             if (req.method == "GET")
//                 req.routeResult = handleRouting(fd, req);
//             return 1;
//         }
//         cout << "still reading body" << endl;
//         return 0;
//     }
//     catch (const HttpExcept& e) {
//         sendErrorResponse(fd, e.getStatusCode(), e.what(), req.conf);
//         requestmp.erase(fd);
//         struct epoll_event ev = {0};
//         ev.data.fd = fd;
//         if (epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, &ev) == -1) {
//             perror("epoll_ctl failed");
//             cout << "epollFd: " << epollFd << ", fd: " << fd << endl;
//         }
//         if (close(fd) == -1)
//             perror("close failed");
//         return -1;
//     }
// }

// void handle_client_read(int clientFd, int epollFd, mpserv& conf, map<int, HttpRequest>& requestmp) {
//     int stat = request(clientFd, conf, epollFd, requestmp);
//     if (stat == 1) {
//         struct epoll_event ev;
//         ev.events = EPOLLOUT;
//         ev.data.fd = clientFd;
//         if (epoll_ctl(epollFd, EPOLL_CTL_MOD, clientFd, &ev) == -1)
//             cout << "epoll ctl error in handle client read\n";
//     }
// }

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

bool HttpRequest::request(int clientFd)
{
    this->clientFd = clientFd;
    char buff[BUFFER_SIZE];
    ssize_t recvBytes = recv(clientFd, buff, BUFFER_SIZE - 1, 0);
    if (recvBytes == 0)
    {
        if (state != COMPLETE && !buffer.empty())
            throw HttpExcept(400, "Bad Request");
        return true;
    }
    else if (recvBytes < 0)
        throw HttpExcept(500, "Internal Server Error");
    if (recvBytes > 0)
    {
        buff[recvBytes] = '\0';
        buffer.append(buff, recvBytes);
        if (state != READING_BODY && buffer.find("\r\n") == string::npos)
            return false;
        if (state == READING_REQUEST_LINE)
            parseRequestLine();
        if (state == READING_HEADERS)
            HandleHeaders();
        if (state == READING_BODY)
            parseBody();
        if (state == COMPLETE)
            return true;
    }
    return false;
}

void sendPostResponse(int clientFd, int statusCode, const string& statusMsg, HttpRequest& req)
{
    string response = "HTTP/1.1 " + to_string(statusCode) + " " + statusMsg + "\r\n";
    response += "Content-Length: 0\r\n";
    response += "Connection: " + req.connection + "\r\n";
    response += "\r\n";
    send(clientFd, response.c_str(), response.size(), 0);
}

void handle_client_read(int clientFd, int epollFd, mpserv& conf, map<int, HttpRequest> &req, map<int , HttpRequest *> &pipes_map)
{
    string host = getInfoClient(clientFd);
    map<int, HttpRequest>::iterator it = req.find(clientFd);
    if (it == req.end())
        req.insert(make_pair(clientFd, HttpRequest(conf.servers[host])));
    try 
    {
        if (req[clientFd].request(clientFd))
        {
            if (req[clientFd].isCGI)
            {
                if (HandleCGI(epollFd, clientFd, req, pipes_map) == -1)
                    throw HttpExcept(500, "Internal Server Error");
                return;
            }
            if (req[clientFd].method == "POST")
            {
                // SEND A 201 OR 204
                sendPostResponse(clientFd, 201, "", req[clientFd]);
                modifyState(epollFd, clientFd, EPOLLIN);
            }
            else if (req[clientFd].method == "GET") {
                req[clientFd].routeResult = handleRouting(clientFd, req[clientFd]);
                modifyState(epollFd, clientFd, EPOLLOUT);

            }            
        }
    }
    catch(const HttpExcept& e)
    {
        sendErrorResponse(clientFd, e.getStatusCode(),e.what(), conf.servers[host]);
        epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, NULL); 
        req.erase(clientFd);
        close(clientFd);
    }
}