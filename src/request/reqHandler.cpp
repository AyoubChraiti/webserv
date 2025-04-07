#include "../../inc/request.hpp"

void parseChecking(const servcnf& server, HttpRequest& req) {
    checkMethod(req.method);
    checkURI(req.path);
    checkHeaders(req);

    string routeName;
    getRoute(server, req.path, routeName, req);

    checkAllowed(req.mtroute, req.method, routeName);
    checkBody(server, req);
}

string HttpRequest::get(const string& key, const string& defaultValue) const {
    map<string, string>::const_iterator it = headers.find(key);
    return (it != headers.end()) ? it->second : defaultValue;
}

void HttpRequest::initFromHeader() {
    host = headers["Host"];
    size_t pos = host.find(":");
    string ip ;//= getIp(host.substr(0, pos)) + ":";
    ip += host.substr(pos + 1);
    host = ip;
    connection = get("Connection", "close");
}

map<int, HttpRequest>::iterator getReqFrmMap(int fd, map<int, HttpRequest>& requestmp) {
    map<int, HttpRequest>::iterator it = requestmp.find(fd);
    if (it == requestmp.end()) {
        requestmp.insert(pair<int, HttpRequest>(fd, HttpRequest()));
        it = requestmp.find(fd);
    }
    return it;
}

string CheckServer(int fd) {
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    
    if (getsockname(fd, (struct sockaddr*)&addr, &addrlen) == -1)
        sysCallFail();
    
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(addr.sin_addr), ip_str, INET_ADDRSTRLEN);
    int port = ntohs(addr.sin_port);
    string result = string(ip_str) + ":" + to_string(port);
    return result;
}

int request(int fd, mpserv& conf, int epollFd, map<int, HttpRequest>& requestmp) {
    map<int, HttpRequest>::iterator it = getReqFrmMap(fd, requestmp);
    string sockHost = CheckServer(fd);
    HttpRequest& req = it->second;
    req.conf = conf.servers[sockHost];
    req.key = sockHost;

    try {
        if (req.parseRequestLineByLine(fd, req.conf)) {
            req.initFromHeader();
            parseChecking(req.conf, req);
            return 1;
        }
        return 0; // still readin
    }
    catch (const HttpExcept& e) {
        sendErrorResponse(fd, e.getStatusCode(), e.what(), req.conf);
        requestmp.erase(fd);
        struct epoll_event ev;
        ev.data.fd = fd;
        epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, &ev);
        close(fd);
        return -1;
    }
}

void handle_client_read(int clientFd, int epollFd, mpserv& conf, map<int, HttpRequest>& requestmp) {
    int stat = request(clientFd, conf, epollFd, requestmp);
    if (stat == 1) { // chunked over
        struct epoll_event ev;
        ev.events = EPOLLOUT;
        ev.data.fd = clientFd;
        epoll_ctl(epollFd, EPOLL_CTL_MOD, clientFd, &ev);
    }
}
