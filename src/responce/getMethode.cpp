#include "../../inc/responce.hpp"
#include "../../inc/request.hpp"

int getMethode(int clientFd, Http* req) {
    RouteResult& routeResult = req->routeResult;

    if (!req->headerSent) {
        sendHeaders(clientFd, routeResult, req);
        return 0;
    }

    if (!req->sendingFile) {
        if (routeResult.shouldRDR) {
            sendRedirect(clientFd, routeResult.redirectLocation, req);
            return 1;
        }    

        if (!routeResult.fileStream) {
            send(clientFd, routeResult.responseBody.c_str(), routeResult.responseBody.size(), 0);
            return 1;
        }
        else {
            req->sendingFile = true;
            req->bytesSentSoFar = 0;
        }
    }

    char buffer[BUFFER_SIZE];
    routeResult.fileStream->read(buffer, BUFFER_SIZE);
    streamsize bytesRead = routeResult.fileStream->gcount();

    if (bytesRead <= 0)
        return 1;

    ssize_t sent = send(clientFd, buffer, bytesRead, 0);
    if (sent < 0)
        return 1;

    req->bytesSentSoFar += sent;

    if ((size_t)sent < (size_t)bytesRead) {
        routeResult.fileStream->seekg(req->bytesSentSoFar, ios::beg);
        return 0;
    }

    if (routeResult.fileStream->eof() || req->bytesSentSoFar >= getContentLength(req->fullPath))
        return 1;

    return 0;
}
