#include "../inc/request.hpp"

bool HttpRequest::parseRequestLineByLine(int fd) {
    char temp[BUFFER_SIZE];
    ssize_t bytes = recv(fd, temp, sizeof(temp) - 1, 0);

    if (bytes > 0) {
        temp[bytes] = '\0';
        buffer += temp;

        while (true) {
            size_t newlinePos = buffer.find("\r\n");
            if (newlinePos == string::npos)
                break; // Wait for more data

            string line = buffer.substr(0, newlinePos);
            buffer.erase(0, newlinePos + 2);

            switch (state) {
                case READING_REQUEST_LINE:
                    {
                        istringstream linestr(line);
                        linestr >> method >> path >> version;

                        if (method.empty() || path.empty() || version.empty())
                            throw HttpExcept(400, "Invalid request line: " + line);

                        if (method != "GET" && method != "POST" && method != "DELETE")
                            throw HttpExcept(501, "Method not implemented: " + method);

                        if (version != "HTTP/1.1")
                            throw HttpExcept(505, "HTTP version not supported: " + version);

                        state = READING_HEADERS;
                    }
                    break;

                case READING_HEADERS:
                    if (line.empty() || line == "\r") {
                        if (headers.find("Host") == headers.end())
                            throw HttpExcept(400, "Host header is required");

                        if (method == "POST") {
                            if (headers.find("Content-Length") == headers.end())
                                throw HttpExcept(411, "Content-Length required for POST");

                            contentLength = strtol(headers["Content-Length"].c_str(), NULL, 10);

                            if (contentLength < 0) {
                                cout << "the content lenght " << contentLength << endl;
                                throw HttpExcept(400, "Invalid Content-Length: negative value");
                            }

                            state = contentLength > 0 ? READING_BODY : COMPLETE;
                        }
                        else {
                            state = COMPLETE;
                        }
                    }
                    else {
                        size_t colpos = line.find(":");
                        if (colpos == string::npos)
                            throw HttpExcept(400, "Malformed header: " + line);

                        string key = trim(line.substr(0, colpos));
                        string value = trim(line.substr(colpos + 1));
                        headers[key] = value;
                    }
                    break;

                case READING_BODY:
                    body += line;
                    bytesRead += line.length();
                    if (bytesRead >= contentLength) {
                        state = COMPLETE;
                        body = body.substr(0, contentLength);
                    }
                    break;

                case COMPLETE:
                    break;
            }
            if (state == COMPLETE)
                return true;
        }
    }
    else if (bytes == 0) {
        if (buffer.empty() && state == READING_REQUEST_LINE)
            throw HttpExcept(400, "Empty request received");

        return state == COMPLETE;
    }
    else {
        return false; // Wait for the next epoll
    }
    return false;
}