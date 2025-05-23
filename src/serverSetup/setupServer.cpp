#include "../../inc/config.hpp"
#include "../../inc/request.hpp"
#include "../../inc/responce.hpp"

#define MAX_EVENTS 10

void add_fds_to_epoll(int epollFd, int fd, uint32_t events) {
    struct epoll_event ev;
    ev.events = events |  EPOLLRDHUP | EPOLLHUP | EPOLLERR;  // Add these flags
    ev.data.fd = fd;

    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        cout << "epoll ctl error in add fds to epoll\n";
        sysCallFail();
    }
}
int get_close_timeout (map<int, time_t> &clientLastActive)
{
    if (clientLastActive.empty())
        return -1;
    
    time_t now = time(NULL);
    std::map<int, time_t>::const_iterator it = clientLastActive.begin();
    int minTime = static_cast<int>(it->second);
    ++it;
    for (; it != clientLastActive.end(); ++it) {
        if (it->second < minTime) {
            minTime = static_cast <int> (it->second);
        }
    }
    int remaining = TIMEOUT - static_cast<int>(now - minTime);
    return remaining * 1000;
}

void epoll_handler(mpserv &conf ,vector<int> &servrs) {
    map<int, Http *> requestmp;
    map<int, Http*> pipes_map;
    map<int, time_t> clientLastActive;
    int epollFd = epoll_create1(0);
    if (epollFd == -1)
        sysCallFail();

    for (size_t i = 0; i < servrs.size(); i++) {
        add_fds_to_epoll(epollFd, servrs[i], EPOLLIN);
    }

    struct epoll_event events[MAX_EVENTS];
    while (!shutServer) {
        int timeout = get_close_timeout(clientLastActive);
        int numEvents = epoll_wait(epollFd, events, MAX_EVENTS, timeout);
        cout << numEvents << endl;
        if (numEvents == -1) {
            if (errno != EINTR) {
                cout << "epoll_wait fail\n";
                sysCallFail();
            }
        }
        
        if (CGImonitor(epollFd, requestmp , pipes_map, clientLastActive))
            continue;
        for (int i = 0; i < numEvents; i++) {
            int eventFd = events[i].data.fd;
            if (find(servrs.begin(), servrs.end(), eventFd) != servrs.end()) {
                struct sockaddr_in clientAddr;
                socklen_t clientLen = sizeof(clientAddr);
                int clientFd = accept(eventFd, (struct sockaddr*)&clientAddr, &clientLen);
                if (clientFd < 0) {
                    cout << "accept failed" << endl;
                    continue;
                }
                add_fds_to_epoll(epollFd, clientFd, EPOLLIN);
                continue;
            }
            if (pipes_map.find(eventFd) != pipes_map.end()) {
                if (events[i].events & EPOLLOUT)
                    handle_cgi_write(eventFd, epollFd, pipes_map, clientLastActive);
                else if (events[i].events & EPOLLIN)
                    handle_cgi_read(epollFd, eventFd, pipes_map[eventFd], pipes_map);
                else if (events[i].events & EPOLLHUP) {
                    clientLastActive.erase(eventFd);
                    pipes_map[eventFd]->stateCGI = COMPLETE_CGI;
                    pipes_map[eventFd]->outputCGI.append("0\r\n\r\n");
                    modifyState(epollFd, pipes_map[eventFd]->clientFd, EPOLLOUT);
                    epoll_ctl(epollFd, EPOLL_CTL_DEL, eventFd, NULL);
                    close(eventFd);
                    pipes_map.erase(eventFd);
                }
                continue;
            }
            if (events[i].events & (EPOLLERR | EPOLLHUP |  EPOLLRDHUP))
            {
                if (!requestmp[eventFd]) {
                    close(eventFd);
                    continue;
                }
                closeFds(epollFd, requestmp, requestmp[eventFd] , pipes_map, clientLastActive);
                continue;
            }
            else {
                if (events[i].events & EPOLLIN)
                {
                    cout << "here" << endl;
                    handle_client_read(eventFd, epollFd, conf, requestmp, pipes_map, clientLastActive);
                }
                 
                else if (events[i].events & EPOLLOUT)
                    handle_client_write(eventFd, epollFd, requestmp, pipes_map, clientLastActive);
            }
        }
    }
    for (map<int, Http*>::iterator it = requestmp.begin(); it != requestmp.end(); ++it) {
        Http* http = it->second;
        if (http && http->routeResult.fileStream != NULL) {
            if (http->routeResult.fileStream->is_open()) {
                http->routeResult.fileStream->close();
            }
            delete http->routeResult.fileStream;
            http->routeResult.fileStream = NULL;
        }
        close(it->first);
        delete http;
    }
    requestmp.clear();

    for (map<int, Http*>::iterator it = pipes_map.begin(); it != pipes_map.end(); ++it) {
        close(it->first);
    }
    pipes_map.clear();

    for (size_t i = 0; i < servrs.size(); i++) {
        close(servrs[i]);
    }
    close(epollFd);
}

void serverSetup(mpserv &conf, vector<int> &servrs) {
    for (map<string, servcnf>::const_iterator it = conf.servers.begin(); it != conf.servers.end(); ++it) {
        int serverFd;
        struct sockaddr_in address;
        if ((serverFd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
            sysCallFail();

        int option = 1;
        if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0)
            sysCallFail();
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = inet_addr(it->second.host.c_str());
        address.sin_port = htons(atoi(it->second.port.c_str()));

        if (bind(serverFd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            cout << "bind failed\n";
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
