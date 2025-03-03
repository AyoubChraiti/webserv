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
        if (req.body.length() > maxSize) {
            throw HttpExcept(413, "Request body too large: " + to_string(req.body.length()) +
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
    connection = get("Connection", "close");
}

map<int, HttpRequest>::iterator getReqFrmMap(int fd, map<int, HttpRequest>& requestStates) {
    map<int, HttpRequest>::iterator it = requestStates.find(fd);
    if (it == requestStates.end()) {
        requestStates.insert(pair<int, HttpRequest>(fd, HttpRequest()));
        it = requestStates.find(fd);
    }
    return it;
}

int request(int fd, mpserv& conf, int epollFd, map<int, HttpRequest>& requestStates) {
    map<int, HttpRequest>::iterator it = getReqFrmMap(fd, requestStates);
    HttpRequest& req = it->second;

    try {
        if (req.parseRequestLineByLine(fd)) {
            req.initFromHeader();
            if (conf.servers.find(req.host) == conf.servers.end()) {
                cerr << "Error: No matching server block for host " << req.host << endl;
                throw HttpExcept(400, "No server configured for host: " + req.host);
            }
            req.conf = conf.servers[req.host];
            parseChecking(req.conf, req);
            return 1; // Request fully parsed
        }
        return 0; // Still parsing
    }
    catch (const HttpExcept& e) {
        sendErrorResponse(fd, e.getStatusCode(), e.what());
        requestStates.erase(fd);
        struct epoll_event ev;
        ev.data.fd = fd;
        epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, &ev);
        return -1;
    }
    catch (const exception& e) {
        sendErrorResponse(fd, 500, "Internal Server Error: " + string(e.what()));
        requestStates.erase(fd);
        struct epoll_event ev;
        ev.data.fd = fd;
        epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, &ev);
        return -1;
    }
}

void handle_client_read(int clientFd, int epollFd, mpserv& conf, map<int, HttpRequest>& requestStates) {
    int stat = request(clientFd, conf, epollFd, requestStates);
    if (stat == 1) {
        struct epoll_event ev;
        ev.events = EPOLLOUT;
        ev.data.fd = clientFd;
        if (epoll_ctl(epollFd, EPOLL_CTL_MOD, clientFd, &ev) == -1) {
            sysCallFail();
        }
    }
    // cout << "the uri in the request: " << requestStates[clientFd].path << endl;
}
