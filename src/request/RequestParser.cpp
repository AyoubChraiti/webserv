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
    // if (uri[0] != '/')
    //     throw RequestException("Bad Request", 400);
    lineLocation = HEAD;
    // uri complete it 
}