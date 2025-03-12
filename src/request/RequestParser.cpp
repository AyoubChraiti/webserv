#include "../../inc/request.hpp"

void HttpRequest::parseRequestLine (servcnf &reqConfig)
{
    size_t index = buffer.find("\r\n");
    if (index == string::npos)
        return ;
    string requsetLine = buffer.substr(0, index);
    if (requsetLine.size() > 2000)
        throw RequestException ("URI Too Long" ,414 );
    stringstream ss (requsetLine);
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
    exit(1);
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
        headers.insert(make_pair(headerline.substr(0, indexColon), trim(headerline.substr(indexColon + 1))));
        buffer.erase(0, index + 2);
    }
    for (auto it = headers.begin(); it != headers.end(); it++)
        cout << it->first << ": " << it->second << endl;
}