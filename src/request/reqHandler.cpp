#include "../../inc/request.hpp"

void sendErrorResponse(int fd, int statusCode, const string& message) {
    string statusText;
    switch (statusCode) {
        case 400: statusText = "Bad Request"; break;
        case 404: statusText = "Not Found"; break;
        case 405: statusText = "Method Not Allowed"; break;
        case 411: statusText = "Length Required"; break;
        case 500: statusText = "Internal Server Error"; break;
        case 501: statusText = "Not Implemented"; break;
        case 505: statusText = "HTTP Version Not Supported"; break;
        default:  statusText = "Error"; break;
    }

    string response =
        "HTTP/1.1 " + to_string(statusCode) + " " + statusText + "\r\n"
        "Content-Length: " + to_string(message.length()) + "\r\n"
        "Content-Type: text/plain\r\n"
        "Connection: close\r\n"
        "\r\n" + message;

    cerr << "Error: '" << message << "' sent to the client" << endl;

    send(fd, response.c_str(), response.length(), 0);
    close(fd);
}

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

map<int, HttpRequest>::iterator getReqFrmMap(int fd) {
    map<int, HttpRequest>::iterator it = requestStates.find(fd);
    if (it == requestStates.end()) {
        requestStates.insert(pair<int, HttpRequest>(fd, HttpRequest()));
        it = requestStates.find(fd);
    }
    return it;
}

int request(int fd, mpserv& conf, int epollFd) {
    map<int, HttpRequest>::iterator it = getReqFrmMap(fd);
    HttpRequest& req = it->second;

    try {
        if (req.parseRequestLineByLine(fd)) {
            req.initFromHeader();
            if (conf.servers.find(req.host) == conf.servers.end()) {
                cerr << "Error: No matching server block for host " << req.host << endl;
                throw HttpExcept(400, "No server configured for host: " + req.host);
            }
            servcnf server = conf.servers[req.host];
            parseChecking(server, req);
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