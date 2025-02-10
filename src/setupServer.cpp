#include "../include/config.hpp"

void handle_request(int client_socket) {

    char buffer[1024];
    read(client_socket, buffer, sizeof(buffer) - 1);
    string request(buffer);

    ifstream html("www/errors/404.html");
    string line;
    string response = "HTTP/1.1 200 K\r\n"
                      "Content-Type: text/html\r\n"
                      "Content-Legth: ""\r\n"
                      "\r\n";

    if (!html.is_open())
        sysCallFail();

    while (getline(html, line)) {
        response += line + "\n";
    }
    send(client_socket, response.c_str(), response.size(), 0);
    close(client_socket);
}

void serverSetup(WebServerConfig serv) {
    int serverFd;
    int clientFd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    serverFd = socket(AF_INET, SOCK_STREAM, 0);

    if (serverFd == 0)
        sysCallFail();

    int option = 1;
    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0)
        sysCallFail();

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(serverFd, (struct sockaddr*)&address, sizeof(address)) < 0)
        sysCallFail();

    if (listen(serverFd, 3) < 0)
        sysCallFail();
    cout << "server listening on port XXXX" << endl;

    socklen_t len = sizeof(address);
    while (true) {
        int client_socket = accept(serverFd, (struct sockaddr*)&address, &len);
        if (client_socket < 0) {
            perror("accept");
            continue;
        }
        handle_request(client_socket);
        cout << "connected\n";
    }
    close(serverFd);
}