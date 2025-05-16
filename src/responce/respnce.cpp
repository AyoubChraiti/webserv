#include "../../inc/responce.hpp"

void sendRedirect(int fd, const string& location, HttpRequest* req) {
    stringstream response;

    response << "HTTP/1.1 301 Moved Permanently\r\n";
    response << "Location: " << location << "\r\n";
    response << "Content-Type: text/html\r\n";
    response << "Content-Length: 0\r\n";
    response << "Connection: " << req->connection << "\r\n";
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

void sendHeaders(int clientFd, RouteResult& routeResult, HttpRequest* req) {
    stringstream response;
    response << "HTTP/1.1 " << routeResult.statusCode << " " << routeResult.statusText << "\r\n";
    response << "Content-Type: " << routeResult.contentType << "\r\n";
    if (routeResult.fileStream) {
        response << "Content-Length: " << getContentLength(routeResult.fullPath) << "\r\n";
    }
    else {
        response << "Content-Length: " << routeResult.responseBody.size() << "\r\n";
    }
    response << "Connection: " << req->connection << "\r\n";
    response << "\r\n";

    send(clientFd, response.str().c_str(), response.str().size(), 0);
    req->headerSent = true;
}

string to_hex(size_t value)
{
    ostringstream oss;
    oss << hex << value;
    return oss.str();
}

void parseCGIandSend(int epollFd, int fd, HttpRequest* req,  map<int, HttpRequest *>& requestmp)
{
    if (req->stateCGI == HEADERS_CGI)
    {
        size_t pos = req->outputCGI.find("\r\n\r\n");
        string headers;
        string body;
        string statusLine = "HTTP/1.1 200 OK\r\n";
        if (pos == string::npos)
        {
            body = req->outputCGI;
            headers.append("Content-Type: text/html\r\n");
            headers.append("Transfer-Encoding: Chunked\r\n");
            headers.append ("Connection: close\r\n");
        }
        else
        {
            map<string , string> storeHeaders;
            headers = req->outputCGI.substr(0, pos);
            body = req->outputCGI.substr(pos + 4);
            size_t start = 0;
            while (start < headers.size())
            {
                size_t posEnd = headers.find("\r\n", start);
                if (posEnd == string::npos)
                    break;
                string line = headers.substr(start, posEnd - start);
                start = posEnd + 2;
                size_t indexColon = line.find(":");
                if (indexColon != string::npos) {
                    string key = trim(line.substr(0, indexColon));
                    string value = trim(line.substr(indexColon + 1));
                    storeHeaders[key] = value;
                }
            }
            if (storeHeaders.count("Content-Type") == 0)
                storeHeaders["Content-Type"] = "text/html";
            if (storeHeaders.count ("Connection") == 0)
                storeHeaders["Connection"] = "close";
            if (storeHeaders.count("Content-Length") > 0)
                storeHeaders.erase("Content-Length");
            if (storeHeaders.count("Transfer-Encoding") > 0)
                storeHeaders.erase("Transfer-Encoding");
            storeHeaders["Transfer-Encoding"] = "chunked";
            headers.clear();
            map<string, string>::const_iterator it = storeHeaders.begin();
            for (; it != storeHeaders.end(); it++)
                headers += it->first + ": " + it->second + "\r\n";

        }
        req->outputCGI.clear();
        req->outputCGI.append(statusLine).append(headers).append("\r\n");
        req->outputCGI.append(to_hex(body.size()) + "\r\n").append(body).append("\r\n");
        req->stateCGI = BODY_CGI;
    }
    else if (req->stateCGI == BODY_CGI)
    {
        string body = req->outputCGI;
        req->outputCGI.clear();
        req->outputCGI.append(to_hex(body.size()) + "\r\n").append(body).append("\r\n");
    }
    send(fd, req->outputCGI.c_str(), req->outputCGI.length(), 0);
    req->outputCGI.clear();
    if (req->stateCGI == COMPLETE_CGI)
    {
        if (epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, NULL) == -1) {
            perror("epoll_ctl");
        }
        delete requestmp[fd];
        requestmp.erase(fd);
        close(fd);
    }
    else
        modifyState(epollFd, fd, EPOLLIN);
}

void handle_client_write(int fd, int epollFd, map<int, HttpRequest *>& requestmp) {
    map<int, HttpRequest*>::iterator it = requestmp.find(fd);
    if (it == requestmp.end()) {
        epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, NULL);
        delete requestmp[fd];
        close(fd);
        return;
    }
    
    HttpRequest* req = it->second;
    try {
        if (req->isCGI) 
            return parseCGIandSend(epollFd, fd, it->second ,requestmp);
        if (!req->mtroute.redirect.empty()) {
            sendRedirect(fd, req->mtroute.redirect, req);
            closeOrSwitch(fd, epollFd, req, requestmp);
            return;
        }
        if (req->method == "GET") {
            int get = getMethode(fd, req);
            if (get) {
                if (req->routeResult.fileStream) {
                    req->routeResult.fileStream->close();
                    delete req->routeResult.fileStream;
                    req->routeResult.fileStream = NULL;
                }
                closeOrSwitch(fd, epollFd, req, requestmp);
                return;
            }
        }
        if (req->method == "DELETE") {
            deleteMethod(fd, req);
            closeOrSwitch(fd, epollFd, req, requestmp);
            return;
        }
    }
    catch (const HttpExcept& e) {
        sendErrorResponse(fd, e.getStatusCode(), e.what(), req->conf);
        delete requestmp[fd];
        requestmp.erase(fd);
        struct epoll_event ev;
        ev.data.fd = fd;
        if (epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, &ev) == -1)
            cout << "epoll ctl error in the client write\n";
        close(fd);
        return;
    }
}