#include "../../inc/responce.hpp"
#include "../../inc/request.hpp"

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
