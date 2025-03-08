#include "../../inc/request.hpp"

void HttpRequest::firstLineParser(string& line) {
    line = trim(line);
    if (line.empty())
        throw HttpExcept(400, "Empty request line");

    cout << "the line in question: " << line << endl;

    size_t firstSpace = line.find(' ');
    if (firstSpace == string::npos)
        throw HttpExcept(400, "Invalid request line: missing method separator");

    size_t secondSpace = line.find(' ', firstSpace + 1);
    if (secondSpace == string::npos || secondSpace <= firstSpace + 1)
        throw HttpExcept(400, "Invalid request line: missing path or version");

    method = line.substr(0, firstSpace);
    path = line.substr(firstSpace + 1, secondSpace - firstSpace - 1);
    version = line.substr(secondSpace + 1);

    if (method.empty())
        throw HttpExcept(400, "Invalid request line: empty method");

    const vector<string> allowedMethods = {"GET", "POST", "DELETE"};
    if (find(allowedMethods.begin(), allowedMethods.end(), method) == allowedMethods.end())
        throw HttpExcept(501, "Method not implemented: " + method);

    if (path.empty() || path[0] != '/')
        throw HttpExcept(400, "Invalid request line: path must be non-empty and start with '/'");

    if (path.find("..") != string::npos)
        throw HttpExcept(400, "Invalid request line: path contains '..' (potential directory traversal)");

    if (version != "HTTP/1.1")
        throw HttpExcept(505, "HTTP version not supported: " + version);

    if (version.size() != 8 || version.substr(0, 5) != "HTTP/" || !isdigit(version[5])
        || version[6] != '.' || !isdigit(version[7])) {
        throw HttpExcept(400, "Invalid request line: malformed HTTP version");
    }
    state = READING_HEADERS;
}

void HttpRequest::HeadersParsing(string& line) {
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
        if (key.empty() || value.empty())
            throw HttpExcept(400, "Malformed header: " + line);

        headers[key] = value;
    }
}

void HttpRequest::bodyPart(string& line) {
    body += line;
    bytesRead += line.size();
    if (bytesRead >= contentLength) {
        state = COMPLETE;
        body = body.substr(0, contentLength);
    }
}

int HttpRequest::Parser(string& line) {
    if (state == READING_REQUEST_LINE) {
        firstLineParser(line);
    }
    else if (state == READING_HEADERS) {
        HeadersParsing(line);
    }
    else if (state == READING_BODY) {
        bodyPart(line);
    }
    if (state == COMPLETE)
        return 1;
    return 0;
}

bool HttpRequest::parseRequestLineByLine(int fd) { // return value is based on if i was done reading or not
    char temp[BUFFER_SIZE];
    ssize_t bytes;
    bytes = recv(fd, temp, sizeof(temp) - 1, 0);
    if (bytes > 0) {
        temp[bytes] = '\0';
        buffer += temp;
        while (true) {
            size_t newlinePos = buffer.find("\n");
            if (newlinePos == string::npos || newlinePos > MAX_LINE) {
                if (buffer.size() > MAX_LINE)
                    throw HttpExcept(501, "Error in request line: line too long " + method);
                break;
            }
            string line = buffer.substr(0, newlinePos);
            buffer.erase(0, newlinePos + 1);

            if (Parser(line))
                return true;
        }
    }
    else if (bytes == 0) { // close connection..
        if (buffer.empty() && state == READING_REQUEST_LINE)
            throw HttpExcept(400, "Empty request received");
    }
    return false;
}
