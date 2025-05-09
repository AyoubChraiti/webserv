#include "../../inc/responce.hpp"

void sendRedirect(int fd, const string& location, HttpRequest& req) {
    stringstream response;

    response << "HTTP/1.1 301 Moved Permanently\r\n";
    response << "Location: " << location << "\r\n";
    response << "Content-Type: text/html\r\n";
    response << "Content-Length: 0\r\n";
    response << "Connection: " << req.connection << "\r\n";
    response << "\r\n";

    string responseStr = response.str();
    size_t bytes = send(fd, responseStr.c_str(), responseStr.size(), 0);

    cout << "bytes sent: " << bytes << endl;
}

size_t getContentLength(const string& path) {
    struct stat fileStat;
    if (stat(path.c_str(), &fileStat) == -1) {
        perror("stat");
        return 0;
    }
    return fileStat.st_size;
}

void sendHeaders(int clientFd, RouteResult& routeResult, HttpRequest& req) {
    stringstream response;
    response << "HTTP/1.1 " << routeResult.statusCode << " " << routeResult.statusText << "\r\n";
    response << "Content-Type: " << routeResult.contentType << "\r\n";

    if (routeResult.resFd != -1) {
        response << "Content-Length: " << getContentLength(routeResult.fullPath) << "\r\n";
    }
    else {
        response << "Content-Length: " << routeResult.responseBody.size() << "\r\n";
    }

    response << "Connection: " << req.connection << "\r\n";
    response << "\r\n";

    cout << "Response Headers: " << response.str() << endl;

    send(clientFd, response.str().c_str(), response.str().size(), 0);
    req.headerSent = true;
}

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
            const string& body = routeResult.responseBody;
            ssize_t sent = send(clientFd, body.c_str(), body.size(), 0);
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

void deleteMethod(int clientFd, int epollFd, HttpRequest& req, map<int, HttpRequest>& requestmp) {
    RouteResult& route = req.routeResult;
    const string& path = route.fullPath;

    if (path.find("..") != string::npos || path.empty()) {
        route.statusCode = 400;
        route.responseBody = "Invalid path";
        sendHeaders(clientFd, route, req);
        return;
    }

    struct stat st;
    if (stat(path.c_str(), &st) != 0) {
        route.statusCode = 404;
        route.responseBody = "Resource not found";
        sendHeaders(clientFd, route, req);
        return;
    }

    if (S_ISDIR(st.st_mode)) {
        if (!req.uri.empty() && req.uri.back() != '/') {
            route.statusCode = 409;
            route.responseBody = "Directory URI must end with '/'";
            sendHeaders(clientFd, route, req);
            return;
        }

        if (!removeDirectoryRecursively(path.c_str())) {
            route.statusCode = 500;
            route.responseBody = "Failed to delete directory";
            sendHeaders(clientFd, route, req);
            return;
        }

        route.statusCode = 204;
        route.responseBody = "";
        sendHeaders(clientFd, route, req);
        return;
    }

    if (access(path.c_str(), W_OK) != 0) {
        route.statusCode = 403;
        route.responseBody = "No write access to file";
        sendHeaders(clientFd, route, req);
        return;
    }

    if (unlink(path.c_str()) != 0) {
        route.statusCode = 500;
        route.responseBody = "Failed to delete file";
        sendHeaders(clientFd, route, req);
        return;
    }

    route.statusCode = 204;
    route.responseBody = "";
    sendHeaders(clientFd, route, req);
}

bool removeDirectoryRecursively(const char* path) {
    DIR* dir = opendir(path);
    if (!dir)
        return false;

    struct dirent* entry;
    while ((entry = readdir(dir))) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        string fullPath = string(path) + "/" + entry->d_name;
        struct stat st;
        if (stat(fullPath.c_str(), &st) != 0) {
            closedir(dir);
            return false;
        }
        if (S_ISDIR(st.st_mode)) {
            if (!removeDirectoryRecursively(fullPath.c_str())) {
                closedir(dir);
                return false;
            }
        } else {
            if (unlink(fullPath.c_str()) != 0) {
                closedir(dir);
                return false;
            }
        }
    }
    closedir(dir);

    if (rmdir(path) != 0)
        return false;
    return true;
}


void handle_client_write(int fd, int epollFd, mpserv& conf, map<int, HttpRequest>& requestmp) {
    HttpRequest& req = requestmp[fd];

    try {
        if (!req.mtroute.redirect.empty()) {
            sendRedirect(fd, req.mtroute.redirect, req);
            closeOrSwitch(fd, epollFd, req, requestmp);
            return;
        }
        if (req.method == "GET") {
            int get = getMethode(fd, epollFd, req, requestmp);
            if (get) {
                int file_fd = req.routeResult.resFd;
                closeOrSwitch(fd, epollFd, req, requestmp);
                if (file_fd != -1)
                    close(file_fd);
                return;
            }
        }
        if (req.method == "DELETE") {
            deleteMethod(fd, epollFd, req, requestmp);
            closeOrSwitch(fd, epollFd, req, requestmp);
            return;
        }
    }
    catch (const HttpExcept& e) {
        sendErrorResponse(fd, e.getStatusCode(), e.what(), req.conf);
        requestmp.erase(fd);
        struct epoll_event ev;
        ev.data.fd = fd;
        if (epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, &ev) == -1)
            cout << "epoll ctl error in the client write\n";
        close(fd);
        return;
    }
}
