#include "../../inc/responce.hpp"
#include "../../inc/request.hpp"

bool removeDirectoryRecursively(const char* path) {
    DIR* dir = opendir(path);
    if (!dir)
        return false;
    struct dirent* entry;
    while ((entry = readdir(dir))) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
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
        }
        else {
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

void sendDeleteResponse(int epollFd, int clientFd, int statusCode, const string& statusMsg, Http* req, map<int, Http*>& requestmp) {
    string response = "HTTP/1.1 " + to_string(statusCode) + " " + statusMsg + "\r\n";
    response += "Content-Length: " + to_string(0) + "\r\n";
    response += "Content-Type: text/plain\r\n";
    response += "Connection: " + req->connection + "\r\n";
    response += "\r\n";
    response += "";

    if (send(clientFd, response.c_str(), response.size(), 0) <= 0) {
        close_connection(clientFd, epollFd, requestmp);
        return;
    }
}

void deleteMethod(int epollFd, int clientFd, Http* req, map<int, Http*>& requestmp) {
    string bestMatch = req->mtroute.root;
    string reqPath = req->uri;

    reqPath.erase(reqPath.begin(), reqPath.begin() + bestMatch.size());
    const string& path = req->mtroute.alias + reqPath;

    if (path.find("..") != string::npos || path.empty())
        throw HttpExcept(400, "invalid path");
    struct stat st;

    if (S_ISDIR(st.st_mode)) {
        if (!req->uri.empty() && back(req->uri) != '/')
            throw HttpExcept(409, "uri must end w /");

        if (!removeDirectoryRecursively(path.c_str()))
            throw HttpExcept(500, "failed to delete file");
        sendDeleteResponse(epollFd, clientFd, 204, "", req, requestmp);
        return;
    }

    if (stat(path.c_str(), &st) != 0)
        throw HttpExcept(404, "not found");

    if (access(path.c_str(), W_OK) != 0)
        throw HttpExcept(403, "file not found");

    if (unlink(path.c_str()) != 0)
        throw HttpExcept(500, "Failed to delete file");

    sendDeleteResponse(epollFd, clientFd, 204, "", req, requestmp);
}
// void sendDeleteResponse(int epollFd, int clientFd, int statusCode, const string& statusMsg, Http* req, map<int, Http*>& requestmp)