#include "../../inc/request.hpp"

void parseChecking(const servcnf& server, HttpRequest& req, int fd) {
    checkMethod(req.method);
    checkURI(req.uri);
    checkHeaders(req);

    string routeName;
    getRoute(server, req.uri, routeName, req);

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
    string ip = host.substr(0, pos) + ":" + host.substr(pos + 1);;
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

    HttpRequest& req = it->second;
    string sockHost = CheckServer(fd);
    if (conf.servers.find(sockHost) == conf.servers.end()) {
        throw HttpExcept(400, "No server for this socket");
    }
    req.conf = conf.servers[sockHost];
    req.key = sockHost;

    try {
        if (req.parseRequestLineByLine(fd, req.conf)) {
            req.initFromHeader();
            parseChecking(req.conf, req, fd);
            if (req.method == "GET")
                req.routeResult = handleRouting(fd, req);
            return 1;
        }
        cout << "still reading body" << endl;
        return 0;
    }
    catch (const HttpExcept& e) {
        sendErrorResponse(fd, e.getStatusCode(), e.what(), req.conf);
        requestmp.erase(fd);
        struct epoll_event ev = {0};
        ev.data.fd = fd;
        if (epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, &ev) == -1) {
            perror("epoll_ctl failed");
            cout << "epollFd: " << epollFd << ", fd: " << fd << endl;
        }
        if (close(fd) == -1)
            perror("close failed");
        return -1;
    }
}

void handle_client_read(int clientFd, int epollFd, mpserv& conf, map<int, HttpRequest>& requestmp) {
    int stat = request(clientFd, conf, epollFd, requestmp);
    if (stat == 1) {
        struct epoll_event ev;
        ev.events = EPOLLOUT;
        ev.data.fd = clientFd;
        if (epoll_ctl(epollFd, EPOLL_CTL_MOD, clientFd, &ev) == -1)
            cout << "epoll ctl error in handle client read\n";
    }
}
