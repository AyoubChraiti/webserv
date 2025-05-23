#include "../../inc/responce.hpp"

void sendRedirect(int epollFd, int fd, const string& location, Http* req, map<int, Http*>& requestmp) {
    stringstream response;

    response << "HTTP/1.1 301 Moved Permanently\r\n";
    response << "Location: " << location << "\r\n";
    response << "Content-Type: text/html\r\n";
    response << "Content-Length: 0\r\n";
    response << "Connection: " << req->connection << "\r\n";
    response << "\r\n";

    string responseStr = response.str();
    if (send(fd, responseStr.c_str(), responseStr.size(), 0) <= 0) {
        close_connection(fd, epollFd, requestmp);
        return;
    }
}

size_t getContentLength(const string& path) {
    struct stat fileStat;
    if (stat(path.c_str(), &fileStat) == -1) {
        perror("stat");
        return 0;
    }
    return fileStat.st_size;
}

void sendHeaders(int epollFd, int clientFd, RouteResult& routeResult, Http* req, map<int, Http*>& requestmp) {
    stringstream response;
    response << "HTTP/1.1 " << routeResult.statusCode << " " << routeResult.statusText << "\r\n";
    response << "Content-Type: " << routeResult.contentType << "\r\n";
    if (routeResult.fileStream) {
        response << "Content-Length: " << getContentLength(req->fullPath) << "\r\n";
    }
    else {
        response << "Content-Length: " << routeResult.responseBody.size() << "\r\n";
    }
    response << "Connection: " << req->connection << "\r\n";
    response << "\r\n";

    if (send(clientFd, response.str().c_str(), response.str().size(), 0) <= 0) {
        close_connection(clientFd, epollFd, requestmp);
        return;
    }
    req->headerSent = true;
}

string to_hex(size_t value) {
    ostringstream oss;
    oss << hex << value;
    return oss.str();
}

void parseCGIandSend(int epollFd, int fd, Http* req,  map<int, Http *>& requestmp)
{
    if (req->stateCGI == HEADERS_CGI)
    {
        int toDelete = 4;
        string lineEnding = "\r\n";
        size_t pos = req->outputCGI.find("\r\n\r\n");
        if (pos == string::npos) {
            toDelete = 2;
            lineEnding = "\n";
            pos =  req->outputCGI.find("\n\n");
        }
        if (pos == string::npos)
            throw HttpExcept(502, "Bad Gateway");

        string headers;
        string body;
        string statusLine = "HTTP/1.1 200 OK\r\n";
        map<string , vector<string> > storeHeaders;

        headers = req->outputCGI.substr(0, pos);
        body = req->outputCGI.substr(pos + toDelete);
        size_t posStatusline = headers.find(lineEnding);
        if (posStatusline != string::npos) {
            if (headers.substr(0, 5) == "HTTP/")
            {
                statusLine = headers.substr(0, posStatusline) + "\r\n";
                headers.erase(0, posStatusline + lineEnding.length());
            }
        }
        size_t start = 0;
        while (start < headers.size())
        {
            size_t posEnd = headers.find(lineEnding, start);
            if (posEnd == string::npos)
                break;

            string line = headers.substr(start, posEnd - start);
            start = posEnd + lineEnding.size();

            size_t indexColon = line.find(":");
            if (indexColon != string::npos) {
                string key = trim(line.substr(0, indexColon));
                string value = trim(line.substr(indexColon + 1));
                storeHeaders[key].push_back(value);
            }
        }
        if (storeHeaders.find("Content-Type") == storeHeaders.end())
            storeHeaders["Content-Type"].push_back("text/html");
        if (storeHeaders.find ("Connection") == storeHeaders.end())
            storeHeaders["Connection"].push_back("close");

        if (storeHeaders.find("Content-Length") != storeHeaders.end())
            storeHeaders.erase("Content-Length");
        if (storeHeaders.find("Transfer-Encoding") != storeHeaders.end())
            storeHeaders.erase("Transfer-Encoding");
        storeHeaders["Transfer-Encoding"].push_back("chunked");
        headers.clear();
        map<string, vector<string> >::const_iterator it = storeHeaders.begin();
        for (; it != storeHeaders.end(); it++)
        {
            vector<string>::const_iterator valuesIt = it->second.begin();
            for (; valuesIt != it->second.end(); ++valuesIt)
                headers += it->first + ": " + *valuesIt + "\r\n";
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
    if (send(fd, req->outputCGI.c_str(), req->outputCGI.length(), 0) <= 0) {
        close_connection(fd, epollFd, requestmp);
        return;
    }
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

void handle_client_write(int fd, int epollFd, map<int, Http *>& requestmp, map<int, Http *> &pipes_map, map<int, time_t>& timer) {
    map<int, Http*>::iterator it = requestmp.find(fd);
    if (it == requestmp.end()) {
        epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, NULL);
        delete requestmp[fd];
        close(fd);
        return;
    }
    
    Http* req = it->second;
    try {
        if (req->routeResult.autoindex) {
            if (!req->headerSent) {
                sendHeaders(epollFd, fd, req->routeResult, req, requestmp);
                return;
            }
            if (send(fd, req->routeResult.responseBody.c_str(), req->routeResult.responseBody.size(), 0) <= 0) {
                close_connection(fd, epollFd, requestmp);
                return;
            }
            closeOrSwitch(fd, epollFd, req, requestmp);
            return;
        }
        else if (req->routeResult.shouldRDR) {
            sendRedirect(epollFd, fd, req->mtroute.redirect, req, requestmp);
            closeOrSwitch(fd, epollFd, req, requestmp);
            return;
        }
        else if (req->isCGI)
            return parseCGIandSend(epollFd, fd, it->second ,requestmp);
        else if (req->method == "GET") {
            int get = getMethode(fd, req, requestmp, epollFd);
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
        else if (req->method == "DELETE") {
            deleteMethod(epollFd, fd, req, requestmp);
            closeOrSwitch(fd, epollFd, req, requestmp);
            return;
        }
    }
    catch (const HttpExcept& e) {
        sendErrorResponse(fd, e.getStatusCode(), e.what(), req->conf);
        if (req->isCGI && req->stateCGI != COMPLETE_CGI) {
            closeFds(epollFd, requestmp, req, pipes_map, timer);
            return ;
        }
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