#include "../../inc/request.hpp"

bool isDirectory(const string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        return (st.st_mode & S_IFDIR) != 0;
    }
    return false;
}

bool fileExists(const string& path) {
    ifstream file(path.c_str());
    bool exists = file.good();
    file.close();
    return exists;
}

class Response {
private:
    int statusCode;
    string statusText;
    map<string, string> headers;
    string body;
    unsigned int contentLength;

public:
    Response() : statusCode(200), statusText("OK"), contentLength(0) {
        headers["Connection"] = "close";
    }

    void setStatus(int code) {
        statusCode = code;
        switch (code) {
            case 200: statusText = "OK"; break;
            case 301: statusText = "Moved Permanently"; break;
            case 403: statusText = "Forbidden"; break;
            case 404: statusText = "Not Found"; break;
            case 405: statusText = "Method Not Allowed"; break;
            case 500: statusText = "Internal Server Error"; break;
            case 501: statusText = "Not Implemented"; break;
            default: statusText = "Unknown"; break;
        }
    }

    void setBody(const string& bodyContent) {
        body = bodyContent;
        contentLength = static_cast<unsigned int>(body.length());
        headers["Content-Length"] = string() + static_cast<char>('0' + contentLength / 1000000 % 10) +
                                   static_cast<char>('0' + contentLength / 100000 % 10) +
                                   static_cast<char>('0' + contentLength / 10000 % 10) +
                                   static_cast<char>('0' + contentLength / 1000 % 10) +
                                   static_cast<char>('0' + contentLength / 100 % 10) +
                                   static_cast<char>('0' + contentLength / 10 % 10) +
                                   static_cast<char>('0' + contentLength % 10);
    }

    void setHeader(const string& key, const string& value) {
        headers[key] = value;
        if (key == "Content-Length") {
            contentLength = 0;
            for (string::size_type i = 0; i < value.length(); ++i) {
                contentLength = contentLength * 10 + (value[i] - '0');
            }
        }
    }

    string generate() const {
        ostringstream response;
        response << "HTTP/1.1 " << statusCode << " " << statusText << "\r\n";
        map<string, string>::const_iterator it;
        for (it = headers.begin(); it != headers.end(); ++it) {
            response << it->first << ": " << it->second << "\r\n";
        }
        response << "\r\n" << body;
        return response.str();
    }

    void send(int fd) const {
        string responseStr = generate();
        ssize_t sent = ::send(fd, responseStr.c_str(), responseStr.length(), 0);
        if (sent < 0) {
            cerr << "Failed to send response to fd " << fd << ": " << strerror(errno) << endl;
        }
        else if (sent < static_cast<ssize_t>(responseStr.length())) {
            cerr << "Partial send to fd " << fd << ": " << sent << " of " << responseStr.length() << endl;
        }
    }

    bool setFile(const string& filePath) {
        ifstream file(filePath.c_str(), ios::binary);
        if (!file.is_open()) {
            return false;
        }
        ostringstream buffer;
        buffer << file.rdbuf();
        setBody(buffer.str());
        file.close();

        string::size_type dotPos = filePath.rfind('.');
        if (dotPos != string::npos) {
            string ext = filePath.substr(dotPos);
            if (ext == ".html") setHeader("Content-Type", "text/html");
            else if (ext == ".txt") setHeader("Content-Type", "text/plain");
            else if (ext == ".css") setHeader("Content-Type", "text/css");
            else if (ext == ".jpg" || ext == ".jpeg") setHeader("Content-Type", "image/jpeg");
            else setHeader("Content-Type", "application/octet-stream");
        }
        else {
            setHeader("Content-Type", "application/octet-stream");
        }
        return true;
    }

    void setRedirect(const string& location) {
        setStatus(301);
        setHeader("Location", location);
        setBody("");
    }

    int getStatusCode() const { return statusCode; }
    unsigned int getContentLength() const { return contentLength; }
    const string& getBody() const { return body; }
};

void handle_client_write(int clientFd, int epollFd, mpserv& conf, map<int, HttpRequest>& requestStates) {
    map<int, HttpRequest>::iterator it = requestStates.find(clientFd);
    if (it == requestStates.end()) {
        Response res;
        res.setStatus(500);
        res.setBody("Internal Error: No request state");
        res.send(clientFd);
        close(clientFd);
        epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, NULL);
        return;
    }

    HttpRequest& req = it->second;
    servcnf& server = req.conf;
    Response res;

    if (req.method != "GET") {
        res.setStatus(501);
        res.setBody("Method Not Implemented: " + req.method);
    }
    else {
        string matchedUri;
        routeCnf* route = 0;
        string::size_type longestMatch = 0;

        map<string, routeCnf>::iterator routeIt;
        for (routeIt = server.routes.begin(); routeIt != server.routes.end(); ++routeIt) {
            const string& uri = routeIt->first;
            if (req.path.find(uri) == 0 && uri.length() > longestMatch) {
                vector<string>& methods = routeIt->second.methodes;
                bool methodAllowed = false;
                for (vector<string>::iterator mIt = methods.begin(); mIt != methods.end(); ++mIt) {
                    if (*mIt == req.method) {
                        methodAllowed = true;
                        break;
                    }
                }
                if (methodAllowed) {
                    matchedUri = uri;
                    route = &routeIt->second;
                    longestMatch = uri.length();
                }
            }
        }

        if (!route) {
            res.setStatus(405);
            res.setBody("No route found for " + req.method + " " + req.path);
        }
        else {
            if (!route->redirect.empty()) {
                res.setRedirect(route->redirect);
            }
            else {
                string remainingPath = (matchedUri == "/" && req.path == "/") ? "" : req.path.substr(matchedUri.length());
                string filePath = route->root + remainingPath;

                if (fileExists(filePath)) {
                    if (isDirectory(filePath) && !route->index.empty()) {
                        string indexPath = filePath + (filePath[filePath.length()-1] == '/' ? "" : "/") + route->index;
                        if (fileExists(indexPath) && !isDirectory(indexPath)) {
                            filePath = indexPath;
                        }
                        else {
                            res.setStatus(403);
                            res.setBody("Directory listing not allowed or no index file");
                        }
                    }

                    if (res.getStatusCode() == 200 && !isDirectory(filePath)) {
                        if (!res.setFile(filePath)) {
                            res.setStatus(500);
                            res.setBody("Failed to open file");
                        }
                    }
                }
                else {
                    res.setStatus(404);
                    res.setBody("Resource not found: " + req.path);
                }
            }
        }
    }
    res.send(clientFd);

    close(clientFd);
    epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, NULL);
    requestStates.erase(clientFd);
}
