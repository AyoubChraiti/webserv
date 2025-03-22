#include "../../inc/config.hpp"
#include "../../inc/request.hpp"
#include "../../inc/responce.hpp"

#define MAX_CLIENTS 1024

void select_handler(mpserv &conf, vector<int> &servrs) {
    map<int, HttpRequest> requestmp;
    fd_set read_fds, write_fds, master_fds;
    int max_fd = 0;

    FD_ZERO(&master_fds);
    for (size_t i = 0; i < servrs.size(); i++) {
        FD_SET(servrs[i], &master_fds);
        if (servrs[i] > max_fd)
            max_fd = servrs[i];
    }

    while (true) {
        read_fds = master_fds;
        write_fds = master_fds;

        int numEvents = select(max_fd + 1, &read_fds, &write_fds, NULL, NULL);
        if (numEvents < 0) {
            if (errno == EINTR) continue; // Ignore signals, retry select()
            perror("select error");
            break;
        }

        vector<int> closed_fds; // Track closed FDs

        for (int fd = 0; fd <= max_fd; fd++) {
            if (FD_ISSET(fd, &read_fds)) {
                if (find(servrs.begin(), servrs.end(), fd) != servrs.end()) {
                    // Accept new connection
                    struct sockaddr_in clientAddr;
                    socklen_t clientLen = sizeof(clientAddr);
                    int clientFd = accept(fd, (struct sockaddr*)&clientAddr, &clientLen);
                    if (clientFd < 0) {
                        perror("accept failed");
                        continue;
                    }
                    FD_SET(clientFd, &master_fds);
                    if (clientFd > max_fd)
                        max_fd = clientFd;
                } else {
                    // Handle read
                    if (handle_client_read(fd, conf, requestmp) < 0) {
                        closed_fds.push_back(fd);
                    }
                }
            }

            if (FD_ISSET(fd, &write_fds)) {
                // Handle write
                if (handle_client_write(fd, requestmp) < 0) {
                    closed_fds.push_back(fd);
                }
            }
        }

        // Remove closed FDs
        for (size_t i = 0; i < closed_fds.size(); i++) {
            int fd = closed_fds[i];
            FD_CLR(fd, &master_fds);
            close(fd);
        }

        // Recalculate max_fd if necessary
        if (!closed_fds.empty() && max_fd > 0) {
            max_fd = *max_element(servrs.begin(), servrs.end());
            for (int fd = 0; fd < FD_SETSIZE; fd++) {
                if (FD_ISSET(fd, &master_fds) && fd > max_fd)
                    max_fd = fd;
            }
        }

        // Check for server shutdown signal
        if (shutServer) {
            cout << "Exiting server successfully" << endl;
            break;
        }
    }
}


void serverSetup(mpserv &conf, vector<int> &servrs) {
    for (auto &server : conf.servers) {
        int serverFd;
        struct sockaddr_in address;
        if ((serverFd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
            sysCallFail();

        int option = 1;
        if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0)
            sysCallFail();

        address.sin_family = AF_INET;
        address.sin_addr.s_addr = inet_addr(server.second.host.c_str());
        address.sin_port = htons(atoi(server.second.port.c_str()));

        if (::bind(serverFd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            cout << "bind failed\n";
            sysCallFail();
        }

        if (listen(serverFd, 10) < 0)
            sysCallFail();

        servrs.push_back(serverFd);
        cout << "server " << server.second.host << " listening on port " << server.second.port << endl;
    }
}

void webserver(mpserv &conf) {
    vector<int> servrs;
    serverSetup(conf, servrs);
    select_handler(conf, servrs);
}
