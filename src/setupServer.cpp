#include "../inc/config.hpp"

#define MAX_EVENTS 10

void add_fds_to_epoll(int epollFd, int fd, uint32_t events) {
    struct epoll_event ev;
    ev.events = events;
    ev.data.fd = fd;

    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &ev) == -1)
        sysCallFail();
}

void handle_client_write(int clientFd, int epollFd, mpserv &conf) { 
    ifstream html("www/errors/404.html");
    string line;
    string response = "HTTP/1.1 200 OK\r\n"
                      "Content-Type: text/html\r\n"
                      "Content-Length: ""\r\n"
                      "\r\n";

    if (!html.is_open())
        sysCallFail();

    while (getline(html, line)) {
        response += line + "\n";
    }

    send(clientFd, response.c_str(), response.length(), 0);

    // Remove from epoll and close the connection
    epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, NULL);
    close(clientFd);
}


void handle_client_read(int clientFd, int epollFd, mpserv &conf) {
    int stat = request(clientFd, conf);
    // Switch to EPOLLIN | EPOLLOUT (so we can read again if needed)
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLOUT;
    ev.data.fd = clientFd;
    epoll_ctl(epollFd, EPOLL_CTL_MOD, clientFd, &ev);
}

void epoll_handler(mpserv &conf ,vector<int> &servrs) {
    int epollFd = epoll_create1(0);
    if (epollFd == -1)
        sysCallFail();

    for (int i = 0; i < servrs.size(); i++) {
        add_fds_to_epoll(epollFd, servrs[i], EPOLLIN);
    }

    struct epoll_event events[MAX_EVENTS];

    while (true) {
        int numEvents = epoll_wait(epollFd, events, MAX_EVENTS, -1);
        if (numEvents == -1)
            sysCallFail();

        for (int i = 0; i < numEvents; ++i) {
            int eventFd = events[i].data.fd;

            if (find(servrs.begin(), servrs.end(), eventFd) != servrs.end()) {
                struct sockaddr_in clientAddr;
                socklen_t clientLen = sizeof(clientAddr);
                int clientFd = accept(eventFd, (struct sockaddr*)&clientAddr, &clientLen);
                if (clientFd < 0) {
                    cout << "accept failed" << endl;
                    continue;
                }
                add_fds_to_epoll(epollFd, clientFd, EPOLLIN | EPOLLET);  // Edge-triggered for efficiency
            }
            else {
                if (events[i].events & EPOLLIN) {
                    handle_client_read(eventFd, epollFd, conf);
                }
                else if (events[i].events & EPOLLOUT) {
                    handle_client_write(eventFd, epollFd, conf);
                }
            }
        }
    }
}

void serverSetup(mpserv &conf, vector<int> &servrs) {
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

        if (bind(serverFd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            cout << "here\n";
            sysCallFail();
        }

        if (listen(serverFd, 10) < 0)
            sysCallFail();

        servrs.push_back(serverFd);
        
        cout << "server " << it->second.host << " listening on port " << it->second.port << endl;
    }
}

void webserver(mpserv &conf) {
    vector<int> servrs;
    serverSetup(conf, servrs);
    epoll_handler(conf, servrs);
}
