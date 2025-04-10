#include "../../inc/request.hpp"
#include "../../inc/responce.hpp"

int request(int fd, mpserv& conf, map<int, HttpRequest>& requestmp) {
    map<int, HttpRequest>::iterator it = getReqFrmMap(fd, requestmp);
    string sockHost = CheckServer(fd);
    HttpRequest& req = it->second;

    // Ensure the server configuration exists for the socket host
    if (conf.servers.find(sockHost) == conf.servers.end()) {
        sendErrorResponse(fd, 400, "No server configured for host: " + sockHost, req.conf);
        requestmp.erase(fd);
        FD_CLR(fd, &master_fds);
        close(fd);
        return -1;
    }

    req.conf = conf.servers[sockHost];
    req.key = sockHost;

    try {
        if (req.parseRequestLineByLine(fd, req.conf)) {
            req.initFromHeader();
            parseChecking(req.conf, req);
            return 1; // Request fully parsed
        }
        return 0; // Still reading the body
    } catch (const HttpExcept& e) {
        sendErrorResponse(fd, e.getStatusCode(), e.what(), req.conf);
        requestmp.erase(fd);
        FD_CLR(fd, &master_fds);
        close(fd);
        return -1;
    } catch (const exception& e) {
        sendErrorResponse(fd, 500, "Internal Server Error: " + string(e.what()), req.conf);
        requestmp.erase(fd);
        FD_CLR(fd, &master_fds);
        close(fd);
        return -1;
    }
}

void handle_client_read(int clientFd, mpserv& conf, map<int, HttpRequest>& requestmp) {
    FD_ZERO(&master_fds);
    FD_ZERO(&write_fds);

    // Add clientFd to the master set (for reading)
    FD_SET(clientFd, &master_fds);

    int stat = request(clientFd, conf, requestmp);
    if (stat == 1) { // Request fully parsed
        FD_SET(clientFd, &write_fds); // Add to write set for response
    } else if (stat == -1) {
        // Error occurred, ensure the file descriptor is removed
        FD_CLR(clientFd, &master_fds);
        FD_CLR(clientFd, &write_fds);
    }
}