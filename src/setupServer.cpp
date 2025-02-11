#include "../inc/config.hpp"

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
    //add_fds_to_epoll();
}

void webserver(mpserv conf) {
    map<int, struct sockaddr_in> servrs; // key= serversFD, value= addInfo;
    serverSetup(conf, servrs);
    // epoll_handler(servrs, serverCongif);
}