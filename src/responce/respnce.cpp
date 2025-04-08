#include "../../inc/responce.hpp"

void sendRedirect(int fd, const string& location, HttpRequest& req) {
    stringstream response;
    cout << "im rederectingg to " << location << endl;

    response << "HTTP/1.1 301 Moved Permanently\r\n";
    response << "Location: " << location << "\r\n";
    response << "Content-Type: text/html\r\n";
    response << "Content-Length: 0\r\n";
    response << "Connection: " << req.connection << "\r\n";
    response << "\r\n";

    string responseStr = response.str();
    send(fd, responseStr.c_str(), responseStr.size(), 0);
}

void getMethode(int clientFd, int epollFd, HttpRequest& req, map<int, HttpRequest>& requestmp) {
    RouteResult routeResult = handleRouting(req);

    if (routeResult.shouldRedirect) {
        sendRedirect(clientFd, routeResult.redirectLocation, req);
        return;
    }

    stringstream response;
    response << "HTTP/1.1 " << routeResult.statusCode << " " << routeResult.statusText << "\r\n";
    response << "Content-Type: " << routeResult.contentType << "\r\n";
    response << "Content-Length: " << routeResult.responseBody.size() << "\r\n";
    response << "Connection: " << req.connection << "\r\n";
    response << "\r\n";
    response << routeResult.responseBody;

    string responseStr = response.str();
    send(clientFd, responseStr.c_str(), responseStr.size(), 0);
}

void handle_client_write(int fd, int epollFd, mpserv& conf, map<int, HttpRequest>& requestmp) {
    HttpRequest req = requestmp[fd];

    try {
        if (!req.mtroute.redirect.empty()) { // handle redirections ..
            sendRedirect(fd, req.mtroute.redirect, req);
            closeOrSwitch(fd, epollFd, req, requestmp);
            return;
        }
        if (req.method == "GET") {
            getMethode(fd, epollFd, req, requestmp);
            closeOrSwitch(fd, epollFd, req, requestmp);
            return;
        }
    }
    catch (const HttpExcept& e) {
        sendErrorResponse(fd, e.getStatusCode(), e.what(), req.conf);
        requestmp.erase(fd);
        struct epoll_event ev;
        ev.data.fd = fd;
        epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, &ev);
        close(fd);
        return;
    }
}
