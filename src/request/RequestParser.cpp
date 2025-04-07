#include "../../inc/request.hpp"
#define MAX_URI_LENGTH 2000

void HttpRequest::parseRequestLine (servcnf &reqConfig)
{
    cout << buffer << endl;
    size_t index = buffer.find("\r\n");
    if (index == string::npos)
        throw RequestException("Bad Request", 400);
    string requestLine = buffer.substr(0, index);
    if (requestLine.size() > MAX_URI_LENGTH)
        throw RequestException ("URI Too Long" ,414);
    stringstream ss (requestLine);
    ss >> method >> uri >> HttpVersion;
    if (!ss.eof())
        throw RequestException("Bad Request", 400);
    if (method.empty() || uri.empty() || HttpVersion.empty())
        throw RequestException("Bad Request", 400);
    if (method != "GET" && method != "POST" && method != "DELETE")
        throw RequestException("Not Implemented", 501);
    if (HttpVersion != "HTTP/1.1")
        throw RequestException("HTTP Version Not Supported", 505);
    buffer.erase(0, index + 2);
    if (reqConfig.routes.find(uri) == reqConfig.routes.end())
        throw RequestException("uri not found 9adha" , 999);
    vector<string>::iterator beginIt = reqConfig.routes[uri].methodes.begin();
    vector<string>::iterator endIt = reqConfig.routes[uri].methodes.end();
    if (find(beginIt, endIt, method) == endIt)
        throw RequestException("Method Not Allowed", 405);
    lineLocation = HEAD;
}
void HttpRequest::parseHeader(servcnf &reqConfig)
{
    size_t index;
    index = 0;
    while ((index = buffer.find("\r\n")) != string::npos)
    {
        if (index == 0)
            break;
        string headerline = buffer.substr(0, index);
        size_t indexColon;
        if ((indexColon = headerline.find(':')) == string::npos)
            throw RequestException("Bad Request", 400);
        string key = trim(headerline.substr(0, indexColon));
        string value = trim(headerline.substr(indexColon + 1));
        headers.insert(std::make_pair(key, value));
        buffer.erase(0, index + 2);
    }
    if (headers.find("Host") == headers.end())
        throw RequestException("Bad Request", 400);
    // if (buffer.find("\r\n\r\n") != string::npos)
    cout << buffer << endl;
    lineLocation = END_REQUEST;
}