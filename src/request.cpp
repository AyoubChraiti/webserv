#include "../inc/config.hpp"

class HttpRequest {
public:
    string method;
    string path;
    string host;
    string connection;
    map<string, string> headers;
    string body;

    HttpRequest(int fd) {
        char buffer[4000];
        ssize_t bytes_received = recv(fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            cerr << "Error receiving request" << endl;
            return;
        }
        buffer[bytes_received] = '\0';
        parseRequest(buffer);
    }

    std::string trim(const std::string &str) {
        size_t start = str.find_first_not_of(" \t\r\n");
        size_t end = str.find_last_not_of(" \t\r\n");

        if (start == std::string::npos || end == std::string::npos)
            return "";

        return str.substr(start, end - start + 1);
    }

    void parseRequest(const string &rawRequest) {
        istringstream requestStream(rawRequest);
        string line;

        if (getline(requestStream, line)) {
            istringstream lineStream(line);
            lineStream >> method >> path;
        }

        while (getline(requestStream, line) && line != "\r") {
            size_t colonPos = line.find(":");
            if (colonPos != string::npos) {
                string key = trim(line.substr(0, colonPos));
                string value = trim(line.substr(colonPos + 1));
                headers[key] = value;
            }
        }
        host = headers["Host"];
        connection = headers["Connection"];

        stringstream bodyStream;
        bodyStream << requestStream.rdbuf();
        body = bodyStream.str();
    }
};

void request(int fd, mpserv &conf) {
    HttpRequest req(fd);

    servcnf server = conf.servers[req.host];
}
