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
        throw HttpExcept(400, "Invalid request line: missing path or version");

    method = trimmedLine.substr(0, firstSpace);
    path = trimmedLine.substr(firstSpace + 1, secondSpace - firstSpace - 1);
    version = trimmedLine.substr(secondSpace + 1);

    if (method.empty() || path.empty() || version.empty())
        throw HttpExcept(400, "Invalid request line");

    state = READING_HEADERS;
}

void HttpRequest::HeadersParsing(const string& line) {
    if (line.empty() || line == "\r") {
        if (headers.find("Host") == headers.end())
            throw HttpExcept(400, "Host header is required");
        
        if (method == "POST") {
            if (headers.find("Content-Length") == headers.end())
                throw HttpExcept(411, "Content-Length required for POST");

            contentLength = strtoul(headers["Content-Length"].c_str(), NULL, 10);
            if (contentLength == 0)
                throw HttpExcept(400, "Invalid Content-Length");

            state = READING_BODY;
        } else {
            state = COMPLETE;
        }
    } else {
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

void HttpRequest::bodyPart(const char* data, size_t length) {
    body.insert(body.end(), data, data + length);
    bytesRead += length;

    if (bytesRead >= contentLength) {
        state = COMPLETE;
        body.resize(contentLength);

        ofstream outFile("file.bin", ios::binary);
        if (!outFile) {
            throw HttpExcept(500, "Failed to open file for writing");
        }
        outFile.write(body.data(), body.size());
        outFile.close();
    }
}

int HttpRequest::Parser(const char* data, size_t length) {
    if (state == READING_REQUEST_LINE) {
        firstLineParser(string(data, length));
    } else if (state == READING_HEADERS) {
        HeadersParsing(string(data, length));
    } else if (state == READING_BODY) {
        bodyPart(data, length);
    }
    return state == COMPLETE ? 1 : 0;
}

bool HttpRequest::parseRequestLineByLine(int fd) {
    char temp[BUFFER_SIZE];
    ssize_t bytes = recv(fd, temp, sizeof(temp), 0);
    if (bytes > 0) {
        buffer.append(temp, bytes);

        while (!buffer.empty()) {
            if (state == READING_REQUEST_LINE || state == READING_HEADERS) {
                size_t newlinePos = buffer.find("\n");
                if (newlinePos == string::npos || newlinePos > MAX_LINE)
                    break;

                string line = buffer.substr(0, newlinePos);
                buffer.erase(0, newlinePos + 1);
                line = trim(line); // Remove trailing \r if needed

                if (Parser(line.c_str(), line.size())) {
                    return true; // Request fully parsed
                }
            } 
            else if (state == READING_BODY) {
                size_t remainingBody = contentLength - bytesRead;
                size_t toRead = min(remainingBody, buffer.size());

                body.insert(body.end(), buffer.begin(), buffer.begin() + toRead);
                buffer.erase(buffer.begin(), buffer.begin() + toRead);
                bytesRead += toRead;

                if (bytesRead >= contentLength) {
                    state = COMPLETE;
                    body.resize(contentLength);

                    // Write to file
                    ofstream outFile("uploaded_file.bin", ios::binary);
                    if (!outFile) {
                        throw HttpExcept(500, "Failed to open file for writing");
                    }
                    outFile.write(body.data(), body.size());
                    outFile.close();

                    return true; // Request is complete
                }
            }
        }
    } 
    else if (bytes == 0 && buffer.empty() && state == READING_REQUEST_LINE) {
        throw HttpExcept(400, "Empty request received");
    }
    return false;
}


// bool HttpRequest::parseRequestLineByLine(int fd) {
//     char temp[BUFFER_SIZE];
//     ssize_t bytes = recv(fd, temp, sizeof(temp), 0);
//     cout << "we here\n";
//     if (bytes > 0) {
//         buffer.append(temp, bytes);
//         while (!buffer.empty()) {
//             size_t newlinePos = buffer.find("\n");
//             if (newlinePos == string::npos || newlinePos > MAX_LINE)
//                 break;
//             string line = buffer.substr(0, newlinePos);
//             buffer.erase(0, newlinePos + 1);

//             if (Parser(line.c_str(), line.size()))
//                 return true;
//         }
//     } else if (bytes == 0 && buffer.empty() && state == READING_REQUEST_LINE) {
//         throw HttpExcept(400, "Empty request received");
//     }
//     return false;
// }
