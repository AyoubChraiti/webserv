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
    outputCGI = "HTTP/1.1 200 OK\r\n";
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
    bodyFile.write(buffer.c_str(), buffer.size());
    buffer.clear();
}
