#include "../../inc/request.hpp"
#define MAX_URI_LENGTH 2000

void HttpRequest::parseRequestLine () // 4. URI path normalization
{
    size_t index = buffer.find("\r\n");
    if (index == string::npos)
        throw HttpExcept(400, "Bad Request");
    string line = buffer.substr(0, index);
    if (line.size() > MAX_URI_LENGTH)
        throw HttpExcept (414, "URI Too Long");
    string requestLine = trim(line);
    stringstream ss (requestLine);
    ss >> method >> uri >> version;
    if (!ss.eof())
        throw HttpExcept(400, "Bad Request");
    if (method.empty() || uri.empty() || version.empty())
        throw HttpExcept(400, "Bad Request");
    if (method != "GET" && method != "POST" && method != "DELETE")
        throw HttpExcept(501, "Not Implemented");
    if (version != "HTTP/1.1")
        throw HttpExcept(505, "HTTP Version Not Supported");
    buffer.erase(0, index + 2);
    if (uri.find("/cgi-bin/") == string::npos)  
    {
        if (uri[0] != '/')
            throw HttpExcept(400, "Bad Request");
        // if (uri.find_first_of("\"<>#%{}|'\\^[]") != string::npos)
        //     throw HttpExcept(400, "Bad Request");
        if (conf.routes.find(uri) == conf.routes.end()) 
        {
            if (conf.routes.find("/") == conf.routes.end())
                throw HttpExcept(404, "Not Found");
            uri = "/";
        }
        vector<string>::iterator beginIt = conf.routes[uri].methodes.begin();
        vector<string>::iterator endIt = conf.routes[uri].methodes.end();
        if (find(beginIt, endIt, method) == endIt)
            throw HttpExcept(405, "Method Not Allowed");
    }
    lineLocation = HEAD;
}
size_t StringStream(string string)
{
    size_t num;
    stringstream ss (string);
    ss >> num;
    return num;
}
void HttpRequest::parseHeader()
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
            throw HttpExcept(400 ,"Bad Request");
        string key = trim(headerline.substr(0, indexColon));
        string value = trim(headerline.substr(indexColon + 1));
        if (key == "Content-Length" || key == "Transfer-Encoding")
        {
            if (key == "Transfer-Encoding")
                isChunked = true;
            else
                sizeBody = StringStream(value);
            isPostKeys = true;
        }
        headers.insert(make_pair(key, value));
        buffer.erase(0, index + 2);
    }
    if (headers.empty() || headers.find("Host") == headers.end())
            throw HttpExcept(400 ,"Bad Request");
    if (method == "POST" && !isPostKeys)
            throw HttpExcept(400 ,"Bad Request");
    if ((method == "GET" || method == "DELETE") && buffer == "\r\n")
        lineLocation = END_REQUEST;
    else
    {
        buffer.erase(0, 2);
        lineLocation = BODY;
    }
}


size_t hexToInt (string str)
{
    return 0;
    // complete this
}

void HttpRequest::parseBody()
{
    if (!isChunked)
    {
        sizeBody -= buffer.size();
        body.append(buffer, buffer.size());
        buffer.erase(0);
        if (sizeBody == 0)
            lineLocation = END_REQUEST;
    }
    else
    {
        while (buffer.size())
        {
            size_t index = buffer.find("\r\n");
            sizeBody =  hexToInt(buffer.substr(0, index));
            // cout << sizeBody << endl;
            if (sizeBody == 0)
                break;
            // buffer.erase(0, index + 2);
            // body.append(buffer, sizeBody);
            // buffer.erase(0);
        }
        // if (sizeBody == 0 )
        //     lineLocation = END_REQUEST;
    }
}