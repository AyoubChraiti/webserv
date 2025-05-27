#include "../../inc/request.hpp"

string getInfoClient(int clientFd) {
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    if (getsockname(clientFd, (struct sockaddr*) &addr, &addrlen))
        sysCallFail();
    char ipStore[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(addr.sin_addr), ipStore, INET_ADDRSTRLEN);
    int port = ntohs(addr.sin_port);
    string host = static_cast<string> (ipStore) + ":" + to_string(port);
    return host;
}

void modifyState(int epollFd ,int clientFd, uint32_t events) {
    struct epoll_event ev;
    ev.events = events ;
    ev.data.fd = clientFd;
    epoll_ctl(epollFd, EPOLL_CTL_MOD, clientFd, &ev);
}

bool Http::request(int Fd) {
    clientFd = Fd;
    char buff[BUFFER_SIZE];
    memset(buff, 0, sizeof(buff));
    ssize_t recvBytes = recv(Fd, buff, BUFFER_SIZE, 0);

    if (recvBytes == 0) {
        if (state == FINISH_REQEUST)
            throw HttpExcept(400, "connection closed by client");
        if (state != COMPLETE && !buffer.empty())
            throw HttpExcept(400, "Bad Request");
        return true;
    }
    else if (recvBytes < 0) {
        throw HttpExcept(500, "Internal Server Error");
    }

    if (recvBytes > 0) {
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

void handle_client_read(int clientFd, int epollFd, mpserv& conf, map<int, Http *> &req, map<int, Http *> &pipes_map, map<int, time_t>& timer) {
    string host = getInfoClient(clientFd);

    if (conf.servers.find(host) == conf.servers.end()) {
        sendErrorResponse(clientFd, 404, "Not Found", conf.servers.begin()->second[0]);
        epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, NULL);
        close(clientFd);
        return;
    }
    
    map<int, Http *>::iterator it = req.find(clientFd);
    if (it == req.end()) {
        const vector<servcnf> &serversForHost = conf.servers[host];

        Http* newReq = NULL;
        if (serversForHost.size() == 1) {
            newReq = new Http(serversForHost[0]);
        }
        else {
            newReq = new Http(serversForHost);
        }
        it = req.insert(make_pair(clientFd, newReq)).first;
    }
    try  {
        if (it->second->request(clientFd)) {
            if (it->second->routeResult.autoindex || it->second->routeResult.shouldRDR) {
                modifyState(epollFd, clientFd, EPOLLOUT);
                it->second->state = FINISH_REQEUST;
                return;
            }
            else if (it->second->isCGI) {
                if (HandleCGI(epollFd, clientFd, req, pipes_map, timer) == -1)
                    throw HttpExcept(500, "Internal Server Error");
                it->second->state = FINISH_REQEUST;
                return;
            }
            else if (it->second->method == "POST") {
                sendPostResponse(clientFd, epollFd,it->second ,req);
                return;
            }
            else if (it->second->method == "GET" || it->second->method == "DELETE") {
                modifyState(epollFd, clientFd, EPOLLOUT);
                it->second->state = FINISH_REQEUST;
                return;
            }
        }
    }
    catch(const HttpExcept& e) {
        if (it->second->state == FINISH_REQEUST) {
            closeFds(epollFd, req, it->second, pipes_map, timer);
            return;
        }
        sendErrorResponse(clientFd, e.getStatusCode(), e.what(), conf.servers[host][0]);
        epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, NULL); 
        delete it->second;
        req.erase(clientFd);
        close(clientFd);
    }
}
