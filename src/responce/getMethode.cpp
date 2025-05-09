#include "../../inc/responce.hpp"
#include "../../inc/request.hpp"

int getMethode(int clientFd, int epollFd, HttpRequest& req, map<int, HttpRequest>& requestmp) {
    RouteResult& routeResult = req.routeResult;

    if (!req.headerSent) {
        sendHeaders(clientFd, routeResult, req);
        return 0;
    }

    if (!req.sendingFile) {
        if (routeResult.shouldRDR) {
            sendRedirect(clientFd, routeResult.redirectLocation, req);
            return 1;
        }    

        if (routeResult.resFd == -1) {
            ssize_t sent = send(clientFd, routeResult.responseBody.c_str(), routeResult.responseBody.size(), 0);
            return 1;
        }
        else {
            req.sendingFile = true;
            req.bytesSentSoFar = 0;
        }
    }
    char buffer[BUFFER_SIZE];

    ssize_t bytesRead = pread(routeResult.resFd, buffer, BUFFER_SIZE, req.bytesSentSoFar);

    ssize_t sent = send(clientFd, buffer, bytesRead, 0);

    req.bytesSentSoFar += sent;

    if ((size_t)sent < (size_t)bytesRead)
        return 0;

    if (req.bytesSentSoFar >= getContentLength(routeResult.fullPath))
        return 1;

    return 0;
}
