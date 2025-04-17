#include "../../inc/request.hpp"
#define MAX_URI_LENGTH 2000

void HttpRequest::HandleUri()
{
    // check this http://localhost:8080/abc/dsadsadas/kapouet/
    map<string , routeCnf>::iterator it = conf.routes.begin();
    size_t prevLength = 0;
    string key;
    bool flag = false; 
    for (; it != conf.routes.end(); it++)
    {
        if (uri.find(it->first) != string::npos && it->first.length() > prevLength) 
        {
            prevLength = it->first.length();
            key = it->first;
            flag = true;
        }
    }
    if (!flag)
        throw HttpExcept(404, "No route for path: " + uri);
    mtroute = conf.routes[key];
}

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
    HandleUri();
    if (mtroute.root == "/cgi-bin/")
        isCGI = true;
    if (find(mtroute.methodes.begin(), mtroute.methodes.end(), method) == mtroute.methodes.end() && !isCGI)
        throw HttpExcept(405, "Method Not Allowed");
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
                contentLength = StringStream(value);
            isPostKeys = true;
        }
        if (key == "Host")
            host = value;
        if (key == "Connection")
            connection = value;
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
    size_t result;
    stringstream ss (str) ;
    ss >> hex >> result;
    return result;
}

void HttpRequest::parseBody()
{
    if (!isChunked)
    {
        contentLength -= buffer.size();
        body.append(buffer.c_str(), buffer.size());
        buffer.clear();
        if (contentLength == 0)
            lineLocation = END_REQUEST;
    }  
    // still need updates (chunked Body)
    else
    {
        size_t index;
        while (buffer.size())
        {
            index = buffer.find("\r\n");
            contentLength =  hexToInt(buffer.substr(0, index));
            if (contentLength == 0)
            {
                lineLocation = END_REQUEST;
                return ;
            }
            buffer.erase(0, index + 2);
            for (size_t i = 0; i < contentLength ; i++)
                bodyChunked.push_back(buffer[i]);
            buffer.erase(0, contentLength + 2);
        }
    }
}