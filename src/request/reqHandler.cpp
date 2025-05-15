#include "../../inc/request.hpp"

string getInfoClient(int clientFd) {
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

void modifyState(int epollFd ,int clientFd, uint32_t events) {
    struct epoll_event ev;
    ev.events = events;
    ev.data.fd = clientFd;
    if (epoll_ctl(epollFd, EPOLL_CTL_MOD, clientFd, &ev) == -1) 
        return ;
}

bool HttpRequest::request(int clientFd) {
    this->clientFd = clientFd;
    char buff[BUFFER_SIZE];
    memset(buff, 0, sizeof(buff));
    ssize_t recvBytes = recv(clientFd, buff, BUFFER_SIZE, 0);
    if (recvBytes == 0) {
        if (state != COMPLETE  && !buffer.empty())
            throw HttpExcept(400, "Bad Request");
        return true;
    }
    else if (recvBytes < 0)
        throw HttpExcept(500, "Internal Server Error");
    if (recvBytes > 0)
    {
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

void handle_client_read(int clientFd, int epollFd, mpserv& conf, map<int, HttpRequest *> &req, map<int, HttpRequest *> &pipes_map) {
    string host = getInfoClient(clientFd);
    map<int, HttpRequest *>::iterator it = req.find(clientFd);
    if (it == req.end()) {
        HttpRequest* newReq = new HttpRequest(conf.servers[host]);
        it = req.insert(make_pair(clientFd, newReq)).first;
    }
    try  {
        if (it->second->request(clientFd)) {
            if (it->second->isCGI) {
                if (HandleCGI(epollFd, clientFd, req, pipes_map) == -1)
                    throw HttpExcept(500, "Internal Server Error");
                return;
            }
            else if (it->second->method == "POST")
                return sendPostResponse(clientFd, epollFd,it->second ,req);
            else if (it->second->method == "GET") {
                it->second->routeResult = handleRouting(it->second);
                modifyState(epollFd, clientFd, EPOLLOUT);
            }
        }
    }
    catch(const HttpExcept& e) {
        sendErrorResponse(clientFd, e.getStatusCode(), e.what(), conf.servers[host]);
        epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, NULL); 
        delete it->second;
        req.erase(clientFd);
        close(clientFd);
    }
}
