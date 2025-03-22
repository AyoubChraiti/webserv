#include "../../inc/request.hpp"

fd_set master_fds; // Set to monitor file descriptors
fd_set write_fds;

void parseChecking(const servcnf& server, const HttpRequest& req) {
    if (req.method != "POST") {
        if (req.path.empty() || req.path[0] != '/')
            throw HttpExcept(400, "Invalid path: " + req.path);

        string matchedRoute;
        const routeCnf* mtRoute = NULL;

        for (map<string, routeCnf>::const_iterator it = server.routes.begin(); it != server.routes.end(); ++it) {
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

int request(int fd, mpserv& conf, map<int, HttpRequest>& requestmp) {
    map<int, HttpRequest>::iterator it = getReqFrmMap(fd, requestmp);
    string sockHost = CheckServer(fd);
    HttpRequest& req = it->second;
    req.conf = conf.servers[sockHost];
    req.key = sockHost;

    try {
        if (conf.servers.find(sockHost) == conf.servers.end())
            throw HttpExcept(400, "No server configured for host: " + req.host);

        if (req.parseRequestLineByLine(fd, req.conf)) {
            req.initFromHeader();
            parseChecking(req.conf, req);
            return 1; // Request fully parsed
        }
        return 0; // Still reading the body
    }
    catch (const HttpExcept& e) {
        sendErrorResponse(fd, e.getStatusCode(), e.what(), req.conf);
        requestmp.erase(fd);
        FD_CLR(fd, &master_fds);
        close(fd);
        return -1;
    }
    catch (const exception& e) {
        sendErrorResponse(fd, 500, "Internal Server Error: " + string(e.what()), req.conf);
        requestmp.erase(fd);
        FD_CLR(fd, &master_fds);
        close(fd);
        return -1;
    }
}

void handle_client_read(int clientFd, mpserv& conf, map<int, HttpRequest>& requestmp) {
    FD_ZERO(&master_fds);
    FD_ZERO(&write_fds);

    // Add clientFd to the master set (for reading)
    FD_SET(clientFd, &master_fds);
    int stat = request(clientFd, conf, requestmp);
    if (stat == 1) { // Request fully parsed
        FD_SET(clientFd, &write_fds);
    }
}