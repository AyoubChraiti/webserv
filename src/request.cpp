#include "../inc/config.hpp"

// class httpexcept : public exception {
// private:
//     int statusCode;
//     string statusMessage;

// public:
//     httpexcept(int code, const string &message)
//         : statusCode(code), statusMessage(message) {}

//     int getStatusCode() const {
//         return statusCode;
//     }

//     const char *what() const throw() {
//         return statusMessage.c_str();
//     }
// };

void sendErrorResponse(int fd, int statusCode, const string &message) {
    string statusText;
    
    switch (statusCode) {
        case 400: statusText = "Bad Request"; break;
        case 404: statusText = "Not Found"; break;
        case 405: statusText = "Method Not Allowed"; break;
        case 500: statusText = "Internal Server Error"; break;
        default:  statusText = "Error"; break;
    }

    string response =
        "HTTP/1.1 " + to_string(statusCode) + " " + statusText + "\r\n"
        "Content-Length: " + to_string(message.length()) + "\r\n"
        "Content-Type: text/plain\r\n"
        "Connection: close\r\n"
        "\r\n" + message;

    send(fd, response.c_str(), response.length(), 0);
    
    close(fd);
}

class HttpRequest {
public:
    char buffer[8000];
    ssize_t req_size;
    string method;
    string path;
    string host;
    string connection;
    map<string, string> headers;
    string body;

    HttpRequest(int fd) {
        req_size = recv(fd, buffer, sizeof(buffer) - 1, 0);
        /*
            buffer;
            string stringbuff = buffer;
            buffer = &(buffer[headerlen]);
        */
        if (req_size <= 0) {
            cerr << "Error receiving request" << endl;
            return;
        }
        buffer[req_size] = '\0';
        parseRequest(buffer);
    }

    string trim(const string &str) {
        size_t start = str.find_first_not_of(" \t\r\n");
        size_t end = str.find_last_not_of(" \t\r\n");

        if (start == string::npos || end == string::npos)
            return "";

        return str.substr(start, end - start + 1);
    }

    string getRequest() const {
        string s;
        for (int i = 0; i < req_size; i++) {
            s += buffer[i];
        }
        return s;
    }

    void parseRequest(const string &rawRequest) {
        istringstream req(rawRequest);
        string line;

        if (getline(req, line)) {
            istringstream linestr(line);
            linestr >> method >> path;
        }

        while (getline(req, line) && line != "\r") {
            size_t colpos = line.find(":");
            if (colpos != string::npos) {
                string key = trim(line.substr(0, colpos));
                string value = trim(line.substr(colpos + 1));
                headers[key] = value;
            }
        }
        host = headers["Host"];
        connection = headers["Connection"];

        stringstream bodyStream;
        bodyStream << req.rdbuf();
        body = bodyStream.str();
    }
};

int request(int fd, mpserv &conf) {
    HttpRequest req(fd);

    if (conf.servers.find(req.host) == conf.servers.end()) {
        cerr << "Error: No matching server block for host " << req.host << endl;
        sendErrorResponse(fd, 400, "Bad Request");
        return 0;
    }

    servcnf server = conf.servers[req.host];
    // parseChecking(server, req.getRequest());
    return 1;
}
