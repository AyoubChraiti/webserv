#include "../../inc/request.hpp"
#define MAX_URI_LENGTH 2000

void HttpRequest::parseRequestLine (servcnf &reqConfig) // 4. URI path normalization
{
    size_t index = buffer.find("\r\n");
    if (index == string::npos)
        throw RequestException("Bad Request", 400);
    string line = buffer.substr(0, index);
    if (line.size() > MAX_URI_LENGTH)
        throw RequestException ("URI Too Long" ,414);
    string requestLine = trim(line);
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
    if (uri != "/cgi-bin/")
    {
        if (uri[0] != '/')
            throw RequestException("Bad Request", 400);
        if (uri.find_first_of("\"<>#%{}|'\\^[]") != string::npos)
            throw RequestException("Bad Request", 400);
        if (reqConfig.routes.find(uri) == reqConfig.routes.end()) 
        {
            if (reqConfig.routes.find("/") == reqConfig.routes.end())
                throw RequestException("Not Found", 404);
            uri = "/";
        }
        vector<string>::iterator beginIt = reqConfig.routes[uri].methodes.begin();
        vector<string>::iterator endIt = reqConfig.routes[uri].methodes.end();
        if (find(beginIt, endIt, method) == endIt)
            throw RequestException("Method Not Allowed", 405);
    }
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
        throw RequestException("Bad Request", 400);
    if (method == "POST" && !isPostKeys)
        throw RequestException("Bad Request", 400);
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

void HttpRequest::parseBody(servcnf &reqConfig)
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