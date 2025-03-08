#include "../../inc/request.hpp"

void parseChecking(const servcnf& server, const HttpRequest& req) {
    if (req.path.empty() || req.path[0] != '/')
        throw HttpExcept(400, "Invalid path: " + req.path);

    string matchedRoute;
    const routeCnf* mtRoute = NULL;

    for (map<string, routeCnf>::const_iterator it = server.routes.begin();
         it != server.routes.end(); ++it) {
        const string& route = it->first;
        const routeCnf& config = it->second;
        if (req.path.find(route) == 0 && route.length() > matchedRoute.length()) {
            matchedRoute = route;
            mtRoute = &config;
        }
    }

    if (mtRoute == NULL)
        throw HttpExcept(404, "No route found for path: " + req.path);

    if (find(mtRoute->methodes.begin(), mtRoute->methodes.end(), req.method)
         == mtRoute->methodes.end()) {
        throw HttpExcept(405, "Method not allowed: " + req.method + " for route " + matchedRoute);
    }

    if (req.method == "POST" && !server.maxBodySize.empty()) {
        size_t maxSize = strtoul(server.maxBodySize.c_str(), NULL, 10);
        if (req.body.size() > maxSize) {
            throw HttpExcept(413, "Request body too large: " + to_string(req.body.size()) +
                                 " exceeds max size " + server.maxBodySize);
        }
    }
}

string HttpRequest::get(const string& key, const string& defaultValue) const {
    map<string, string>::const_iterator it = headers.find(key);
    return (it != headers.end()) ? it->second : defaultValue;
}

void HttpRequest::initFromHeader() {
    host = headers["Host"];
    size_t pos = host.find(":");
    string ip = getIp(host.substr(0, pos)) + ":";
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
    HttpRequest& req = it->second;

    string sockHost = CheckServer(fd);

    try {
        if (conf.servers.find(sockHost) == conf.servers.end())
            throw HttpExcept(400, "No server configured for host: " + req.host);

        if (req.parseRequestLineByLine(fd)) {
            req.initFromHeader();

            req.conf = conf.servers[req.host];
            parseChecking(req.conf, req);

            return 1; // Request fully parsed
        }
        return 0; // still reading the body mr sir
    }
    catch (const HttpExcept& e) {
        sendErrorResponse(fd, e.getStatusCode(), e.what());
        requestmp.erase(fd);
        struct epoll_event ev;
        ev.data.fd = fd;
        epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, &ev);
        return -1;
    }
    catch (const exception& e) {
        sendErrorResponse(fd, 500, "Internal Server Error: " + string(e.what()));
        requestmp.erase(fd);
        struct epoll_event ev;
        ev.data.fd = fd;
        epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, &ev);
        return -1;
    }
}

void handle_client_read(int clientFd, int epollFd, mpserv& conf, map<int, HttpRequest>& requestmp) {
    int stat = request(clientFd, conf, epollFd, requestmp);
    if (stat == 1) { // means we done from the req.
        struct epoll_event ev;
        ev.events = EPOLLOUT;
        ev.data.fd = clientFd;
        if (epoll_ctl(epollFd, EPOLL_CTL_MOD, clientFd, &ev) == -1) {
            sysCallFail();
        }
    }
    // cout << "the uri in the request: " << requestmp[clientFd].path << endl;
}
