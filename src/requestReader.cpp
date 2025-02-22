#include "../inc/request.hpp"
#include <sstream>
#include <unistd.h>
#include <sys/epoll.h>

#define BUFFER_SIZE 1024

bool parseRequestLineByLine(int fd, RequestParser& parser) {
    char temp[BUFFER_SIZE];
    ssize_t bytes = recv(fd, temp, sizeof(temp) - 1, 0);

    if (bytes > 0) {
        temp[bytes] = '\0';
        parser.buffer += temp;

        while (true) {
            size_t newlinePos = parser.buffer.find("\r\n");
            if (newlinePos == string::npos)
                break; // Wait for more data

            string line = parser.buffer.substr(0, newlinePos);
            parser.buffer.erase(0, newlinePos + 2);

            switch (parser.state) {
                case READING_REQUEST_LINE:
                    {
                        istringstream linestr(line);
                        linestr >> parser.method >> parser.path >> parser.version;

                        if (parser.method.empty() || parser.path.empty() || parser.version.empty())
                            throw HttpExcept(400, "Invalid request line: " + line);

                        if (parser.method != "GET" && parser.method != "POST" && parser.method != "DELETE")
                            throw HttpExcept(501, "Method not implemented: " + parser.method);

                        if (parser.version != "HTTP/1.1")
                            throw HttpExcept(505, "HTTP version not supported: " + parser.version);

                        parser.state = READING_HEADERS;
                    }
                    break;

                case READING_HEADERS:
                    if (line.empty() || line == "\r") {
                        if (parser.headers.find("Host") == parser.headers.end())
                            throw HttpExcept(400, "Host header is required");

                        if (parser.method == "POST") {
                            if (parser.headers.find("Content-Length") == parser.headers.end())
                                throw HttpExcept(411, "Content-Length required for POST");

                            parser.contentLength = strtol(parser.headers["Content-Length"].c_str(), NULL, 10);

                            if (parser.contentLength < 0)
                                throw HttpExcept(400, "Invalid Content-Length: negative value");

                            parser.state = parser.contentLength > 0 ? READING_BODY : COMPLETE;
                        }
                        else {
                            parser.state = COMPLETE;
                        }
                    }
                    else {
                        size_t colpos = line.find(":");
                        if (colpos == string::npos)
                            throw HttpExcept(400, "Malformed header: " + line);

                        string key = trim(line.substr(0, colpos));
                        string value = trim(line.substr(colpos + 1));
                        parser.headers[key] = value;
                    }
                    break;

                case READING_BODY:
                    parser.body += line;
                    parser.bytesRead += line.length();
                    if (parser.bytesRead >= parser.contentLength) {
                        parser.state = COMPLETE;
                        parser.body = parser.body.substr(0, parser.contentLength);
                    }
                    break;

                case COMPLETE:
                    break;
            }

            if (parser.state == COMPLETE)
                return true;
        }
    }
    else if (bytes == 0) {
        if (parser.buffer.empty() && parser.state == READING_REQUEST_LINE)
            throw HttpExcept(400, "Empty request received");

        return parser.state == COMPLETE;
    }
    else {
        return false; // Wait for the next epoll
    }

    return false;
}