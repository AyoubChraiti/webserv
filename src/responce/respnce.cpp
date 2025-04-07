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

string generateAutoIndex(const string& fullPath, const string& uriPath) {
    stringstream html;

    html << "<html><head><title>Index of " << uriPath << "</title></head><body>";
    html << "<h1>Index of " << uriPath << "</h1><hr><pre>";

    DIR* dir = opendir(fullPath.c_str());
    if (!dir) {
        return "<h1>403 Forbidden</h1>";
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        string name = entry->d_name;

        // Skip "." but include ".."
        if (name == ".")
            continue;

        string displayName = name;
        string href = uriPath;
        if (href.back() != '/') href += "/";
        href += name;

        string fullEntryPath = fullPath + "/" + name;

        struct stat st;
        if (stat(fullEntryPath.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
            displayName += "/";
            href += "/";
        }

        html << "<a href=\"" << href << "\">" << displayName << "</a>\n";
    }
    closedir(dir);
    html << "</pre><hr></body></html>";
    return html.str();
}


void getMethode(int clientFd, int epollFd, HttpRequest& req, map<int, HttpRequest>& requestmp) {
    string responseBody;
    string contentType = "text/plain";
    int statusCode = 200;
    string statusText = "OK";

    // Find the best matching route from req.conf.routes
    string matchedPath;
    routeCnf* matchedRoute = nullptr;
    for (const auto& [prefix, route] : req.conf.routes) {
        if (req.path.find(prefix) == 0 &&
            (matchedRoute == nullptr || prefix.length() > matchedPath.length())) {
            matchedRoute = const_cast<routeCnf*>(&route); // store pointer to matched route
            matchedPath = prefix;
        }
    }

    if (!matchedRoute) {
        cerr << "No matching route found, using 404." << endl;
        statusCode = 404;
        statusText = "Not Found";
        responseBody = "404 Not Found";
    } else {
        req.mtroute = *matchedRoute; // store matched route in request (if you need it later)

        // Handle redirect
        if (!matchedRoute->redirect.empty()) {
            stringstream res;
            res << "HTTP/1.1 301 Moved Permanently\r\n";
            res << "Location: " << matchedRoute->redirect << "\r\n";
            res << "Content-Length: 0\r\n";
            res << "Connection: close\r\n\r\n";
            send(clientFd, res.str().c_str(), res.str().size(), 0);
            return;
        }

        string relativePath = req.path.substr(matchedPath.length());
        if (!relativePath.empty() && relativePath[0] == '/')
            relativePath = relativePath.substr(1); // remove leading slash

        string fullPath = matchedRoute->root + "/" + relativePath;

        // Handle directory with index
        if (isDirectory(fullPath)) {
            if (req.path.back() != '/') {
                sendRedirect(clientFd, req.path + "/", req);
                return;
            }

            if (!matchedRoute->index.empty()) {
                fullPath += matchedRoute->index[0] == '/' ? matchedRoute->index : "/" + matchedRoute->index;
            } else if (matchedRoute->autoindex) {
                responseBody = generateAutoIndex(fullPath, req.path); // custom function
                contentType = "text/html";
            } else {
                statusCode = 403;
                statusText = "Forbidden";
                responseBody = "403 Forbidden";
            }
        }

        // Serve file if not already handled
        if (statusCode == 200 && responseBody.empty()) {
            ifstream file(fullPath, ios::binary);
            if (!file.is_open()) {
                statusCode = 404;
                statusText = "Not Found";
                responseBody = "404 Not Found";
            } else {
                stringstream buffer;
                buffer << file.rdbuf();
                responseBody = buffer.str();
                contentType = getContentType(fullPath); // based on extension
                file.close();
            }
        }
    }

    // Compose response
    stringstream response;
    response << "HTTP/1.1 " << statusCode << " " << statusText << "\r\n";
    response << "Content-Type: " << contentType << "\r\n";
    response << "Content-Length: " << responseBody.size() << "\r\n";
    response << "Connection: " << req.connection << "\r\n";
    response << "\r\n";
    response << responseBody;

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
