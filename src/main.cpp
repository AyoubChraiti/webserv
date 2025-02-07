#include "../include/header.hpp"
#include "../include/config.hpp"

void handle_request(int client_socket) {
    char buffer[1024];
    read(client_socket, buffer, sizeof(buffer) - 1);
    string request(buffer);

    string html_content = "<html><body><h1>simo</h1></body></html>";
    string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: text/html\r\n";
    response += "Content-Length: " + to_string(html_content.size()) + "\r\n";
    response += "\r\n";
    response += html_content;

    send(client_socket, response.c_str(), response.size(), 0);
    close(client_socket);
}

int main(int ac, char **av) {
    if (ac != 2) {
        cout << "Usage: ./webserv <config_file>" << endl;
        cout << "Curently using default path" << endl;
        // default_path_init();
    }
    else {
        config_file(av[1]);
    }
//     int serverFd, clientFd;
//     struct sockaddr_in address;
//     int addrlen = sizeof(address);
//     serverFd = socket(AF_INET, SOCK_STREAM, 0);
//     if (serverFd == 0) {
//         cerr << "socket failed" << endl;
//         return 1;
//     }

//     int option = 1;
//     if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0) {
//         cerr << "setsockopt failed" << endl;
//         return 1;
//     }

//     address.sin_family = AF_INET;
//     address.sin_addr.s_addr = INADDR_ANY;
//     address.sin_port = htons(8080);

//     if (bind(serverFd, (struct sockaddr*)&address, sizeof(address)) < 0) {
//         perror("bind");
//         return 1;
//     }

//     if (listen(serverFd, 3) < 0) {
//         perror("listen");
//         return 1;
//     }
//     cout << "server listening on port 8080" << endl;

//     socklen_t len = sizeof(address);
//     while (true) {
//         int client_socket = accept(serverFd, (struct sockaddr*)&address, &len);
//         if (client_socket < 0) {
//             perror("accept");
//             continue;
//         }
//         handle_request(client_socket);
//     }
//     close(serverFd);
}