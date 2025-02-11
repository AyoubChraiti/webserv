#include "../inc/config.hpp"

#define MAX_EVENTS 10

void add_fds_to_epoll(int epollFd, int serverFd) {
    struct epoll_event ev;
    ev.events = EPOLLIN; // will listen for read only???
    ev.data.fd = serverFd;
    
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, serverFd, &ev) == -1)
        sysCallFail();
    cout << "FDDD= " << serverFd << endl;
}

void handle_client_write(int clientFd, int epollFd) {
    const char response[] = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, world!";
    send(clientFd, response, sizeof(response) - 1, 0);

    // Remove client from epoll and close socket after response
    epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, NULL);
    close(clientFd);
}


void handle_client_read(int clientFd, int epollFd) {
    char buffer[4096];
    int bytesRead = recv(clientFd, buffer, sizeof(buffer), 0);

    if (bytesRead <= 0) {
        // Client closed connection or error
        close(clientFd);
        epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, NULL);
        return;
    }

    cout << "Received request from client " << clientFd << ": " << buffer << endl;

    // Switch to EPOLLOUT for response
    struct epoll_event ev;
    ev.events = EPOLLOUT;
    ev.data.fd = clientFd;
    epoll_ctl(epollFd, EPOLL_CTL_MOD, clientFd, &ev);
}


void epoll_handler(mpserv conf ,map<int, struct sockaddr_in> &servrs) {
    int epollFd = epoll_create1(0);
    if (epollFd == -1)
        sysCallFail();

    for (const auto &entry : servrs) {
        add_fds_to_epoll(epollFd, entry.first);
    }

    struct epoll_event events[MAX_EVENTS];

    while (true) {
        int numEvents = epoll_wait(epollFd, events, MAX_EVENTS, -1);
        if (numEvents == -1)
            sysCallFail();

        for (int i = 0; i < numEvents; ++i) {
            int eventFd = events[i].data.fd;

            if (servrs.find(eventFd) != servrs.end()) {
                // It's a server socket, accept new client
                struct sockaddr_in clientAddr;
                socklen_t clientLen = sizeof(clientAddr);
                int clientFd = accept(eventFd, (struct sockaddr*)&clientAddr, &clientLen);
                if (clientFd < 0) {
                    cout << "accept failed" << endl;
                    continue;
                }

                cout << "New client connected: " << clientFd << endl;
                cerr << clientFd << "new client connected to : " << eventFd << endl;

                // Set client socket to non-blocking
                // fcntl(clientFd, F_SETFL, O_NONBLOCK);

                // Add client to epoll
                add_fds_to_epoll(epollFd, clientFd);
            }
            else {
                // Handle existing client requests
                if (events[i].events & EPOLLIN) {
                    cout << eventFd << endl;
                    handle_client_read(eventFd, epollFd);
                }
                else if (events[i].events & EPOLLOUT) {
                    handle_client_write(eventFd, epollFd);
                }
            }
        }
    }
}

void serverSetup(mpserv conf, map<int, struct sockaddr_in> &servrs) {
    for (map<string, servcnf>::iterator it = conf.servers.begin(); it != conf.servers.end(); ++it) {
        int serverFd;
        struct sockaddr_in address;
        int addrlen = sizeof(address);
        if ((serverFd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
            sysCallFail();

        int option = 1;
        if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0)
            sysCallFail();

        address.sin_family = AF_INET;
        address.sin_addr.s_addr = inet_addr(it->second.host.c_str());
        address.sin_port = htons(atoi(it->second.port.c_str()));

        if (bind(serverFd, (struct sockaddr*)&address, sizeof(address)) < 0)
            sysCallFail();

        if (listen(serverFd, 3) < 0)
            sysCallFail();

        servrs[serverFd] = address;
        
        cout << "server listening on port " << it->second.port << endl;
    }
}

void webserver(mpserv conf) {
    map<int, struct sockaddr_in> servrs;
    serverSetup(conf, servrs);
    epoll_handler(conf, servrs);
}
