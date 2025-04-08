#include "../../inc/request.hpp"

void HttpRequest::firstLineParser(const string& line) {
    string trimmedLine = trim(line);
    if (trimmedLine.empty())
        throw HttpExcept(400, "Empty request line");

    size_t firstSpace = trimmedLine.find(' ');
    if (firstSpace == string::npos)
        throw HttpExcept(400, "Invalid request line: missing method separator");

    size_t secondSpace = trimmedLine.find(' ', firstSpace + 1);
    if (secondSpace == string::npos || secondSpace <= firstSpace + 1)
        throw HttpExcept(400, "Invalid request line: missing uri or version");

    method = trimmedLine.substr(0, firstSpace);
    uri = trimmedLine.substr(firstSpace + 1, secondSpace - firstSpace - 1);
    version = trimmedLine.substr(secondSpace + 1);

    if (method.empty() || uri.empty() || version.empty())
        throw HttpExcept(400, "Invalid request line");

    state = READING_HEADERS;
}

void HttpRequest::HeadersParsing(const string& line) {
    if (line.empty() || line == "\r") {
        if (headers.find("Host") == headers.end())
            throw HttpExcept(400, "Host header is required");

        if (method == "POST") {
            auto it = headers.find("Content-Length");
            if (it == headers.end()) {
                throw HttpExcept(411, "Content-Length required for POST");
            }
            contentLength = stoul(it->second);
            if (contentLength == 0) {
                throw HttpExcept(400, "Invalid Content-Length");
            }
            state = READING_BODY;
        }
        else
            state = COMPLETE;
    }
    else {
        size_t pos = line.find(":");
        if (pos == string::npos)
            throw HttpExcept(400, "Malformed header: " + line);

        string key = line.substr(0, pos);
        string value = line.substr(pos + 1);
        key.erase(remove_if(key.begin(), key.end(), ::isspace), key.end());
        value.erase(0, value.find_first_not_of(" \t"));

        if (key.empty() || value.empty())
            throw HttpExcept(400, "Malformed header: " + line);
        headers[key] = value;
    }
}

void HttpRequest::bodyPart(const char* data, size_t length, servcnf& conf) {
    size_t remaining = contentLength - bytesRead;
    size_t toWrite = min(remaining, length);
    body.insert(body.end(), data, data + toWrite);
    bytesRead += toWrite;

    if (bytesRead >= contentLength) {
        state = COMPLETE;
    }
}

int HttpRequest::Parser(const char* data, size_t length) {
    switch (state) {
        case READING_REQUEST_LINE:
            firstLineParser(string(data, length));
            break;
        case READING_HEADERS:
            HeadersParsing(string(data, length));
            break;
        case READING_BODY:
            bodyPart(data, length, conf);
            break;
        default:
            return 1;
    }
    return state == COMPLETE ? 1 : 0;
}

bool HttpRequest::parseRequestLineByLine(int fd, servcnf& conf) {
    char temp[BUFFER_SIZE];
    ssize_t bytes = recv(fd, temp, sizeof(temp), 0);

    if (bytes == 0)
        return true;
    else if (bytes < 0)
        throw HttpExcept(400, "Empty or invalid request");

    buffer.append(temp, bytes);
    while (!buffer.empty()) {
        if (state == READING_REQUEST_LINE || state == READING_HEADERS) {
            size_t newlinePos = buffer.find("\n");
            if (newlinePos == string::npos || newlinePos > MAX_LINE) {
                throw HttpExcept(400, "Invalid line in the request");
            }
            string line = buffer.substr(0, newlinePos);
            buffer.erase(0, newlinePos + 1);
            line.erase(remove(line.begin(), line.end(), '\r'), line.end());

            if (Parser(line.c_str(), line.size()))
                return true;
        }
        else if (state == READING_BODY) {
            bodyPart(buffer.data(), buffer.size(), conf);
            buffer.clear();
            if (state == COMPLETE)
                return true;
        }
    }
    return false;
}
