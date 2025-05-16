#include "../../inc/request.hpp"

HttpRequest::HttpRequest(servcnf config) {
    contentLength = 0;
    bytesRead = 0;
    sendingFile = false;
    bytesSentSoFar = 0;
    headerSent = 0;
    state = READING_REQUEST_LINE;
    isPostKeys = true;
    isChunked = false;
    isCGI = false;
    conf = config;
    remaining = 0;
    querystring = "";
    Boundary = "";
    startBoundFlag = false;
    hasBody = false;
    stateCGI = HEADERS_CGI;
    stdinFd = -1;
    stdoutFd = -1;
    clientFd = -1;
    cgiPid = -1;
}

HttpRequest::~HttpRequest() {}

size_t StringStream(const string &string)
{
    size_t num;
    stringstream ss (string);
    ss >> num;
    return num;
}

bool is_file(const string& path) {
    struct stat buffer;
    if (stat(path.c_str(), &buffer) != 0) {
        return false;
    }
    return S_ISREG(buffer.st_mode);
}

bool isValidContentLength(const string &value) {
    for (size_t i = 0; i < value.length(); ++i) {
        if (!isdigit(value[i])) {
            return false;
        }
    }
    return true;
}

size_t hexToInt (const string &str) {
    size_t result;
    stringstream ss (str) ;
    ss >> hex >> result;
    return result;
}

string getFileName(string buff) {
    size_t header_end = buff.find("\r\n\r\n");
    if (header_end == string::npos)
        throw HttpExcept(400, "Bad Request1");
    size_t start_filename = buff.find ("filename=\"");
    if (start_filename == string::npos)
        throw HttpExcept(400, "Bad Request2");
    start_filename += 10;
    size_t end_filename = buff.find("\r\n");
    if (end_filename == string::npos)
        throw HttpExcept(400, "Bad Request3");   
    return (buff.substr(start_filename , end_filename - start_filename - 1));   
}

void writebody(fstream &bodyFile , string &buffer) {
    if (buffer.empty ())
        return ;
    bodyFile.write(buffer.c_str(), buffer.size());
    buffer.clear();
}

void sendPostResponse(int clientFd, int epollFd, HttpRequest* req, map<int, HttpRequest *> &reqStates) {
    string response;
    response.append("HTTP/1.1 ").append(req->hasBody ? "200 OK" : "204 No Content").append("\r\n");
    response.append("Connection: ").append(req->connection).append("\r\n");
    if (req->hasBody)
    {
        string body =
            "<!DOCTYPE html>\r\n"
            "<html lang=\"en\">\r\n"
            "<head>\r\n"
            "  <meta charset=\"UTF-8\">\r\n"
            "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\r\n"
            "  <title>Upload Result</title>\r\n"
            "</head>\r\n"
            "<body>\r\n"
            "  <h1>File upload successful</h1>\r\n"
            "</body>\r\n"
            "</html>\r\n";
        response.append("Content-Type: text/html; charset=UTF-8\r\n");
        response.append("Content-Length: ").append(to_string(body.size())).append("\r\n");
        response.append("\r\n");
        response.append(body);

    }
    else
    {
        string body =
            "<!DOCTYPE html>\r\n"
            "<html lang=\"en\">\r\n"
            "<head>\r\n"
            "  <meta charset=\"UTF-8\">\r\n"
            "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\r\n"
            "  <title>No Content</title>\r\n"
            "</head>\r\n"
            "<body>\r\n"
            "  <h1>No body to upload message</h1>\r\n"
            "</body>\r\n"
            "</html>\r\n";
        response.append("Content-Type: text/html; charset=UTF-8\r\n");
        response.append("Content-Length: ").append(to_string(body.size())).append("\r\n");
        response.append("\r\n");
        response.append(body);
    }
    send(clientFd, response.c_str(),response.size(), 0);
    closeOrSwitch(clientFd, epollFd, req, reqStates);
}