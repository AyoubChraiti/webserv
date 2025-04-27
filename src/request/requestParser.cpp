#include "../../inc/request.hpp"
#define MAX_URI_LENGTH 2000

void HttpRequest::HandleUri()
{
    if (method == "GET" || method == "DELETE")
    {
        size_t indexQUERY = uri.find("?");
        if (indexQUERY != string::npos)
        {
            querystring =  uri.substr(indexQUERY + 1);
            uri.erase(indexQUERY);
        }
    }
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
    if (mtroute.root == "/cgi-bin/") // check later
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
        if (key == "Content-Type")
        {
            size_t pos;
            if ((pos = value.find("boundary=")) != string::npos)
                Boundary =  value.substr(pos + 9);
        }
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
        lineLocation = BODY;
        buffer.erase(0, 2);
        if (Boundary.empty())
            openFile("bigfile.txt");
    }
}

size_t hexToInt (string str)
{
    size_t result;
    stringstream ss (str) ;
    ss >> hex >> result;
    return result;
}

void HttpRequest::HandleChunkedBody()
{
    while (!buffer.empty())
    {
        size_t crlf_pos;
        if (remaining == 0)
        {
            crlf_pos = buffer.find("\r\n");
            if (crlf_pos == std::string::npos) 
                throw HttpExcept(400 ,"Bad Request");
            contentLength = hexToInt(buffer.substr(0, crlf_pos));
            buffer.erase(0, crlf_pos + 2);
        }
        else
            contentLength = remaining;
        if (contentLength == 0)
        {
            buffer.clear();
            bodyFile.clear();
            bodyFile.seekg(0, ios::beg);
            lineLocation = END_REQUEST;
            break;
        }
        size_t bytesTowrite = min(contentLength, buffer.size());
        bodyFile.write(buffer.c_str(), bytesTowrite);
        (contentLength > buffer.size()) ? buffer.erase(0, buffer.size()) : buffer.erase(0, contentLength);
        remaining = contentLength - bytesTowrite;
        if (!remaining)
            (buffer.find("\r\n") == string::npos) ? throw HttpExcept(400 ,"Bad Request") : buffer.erase(0, 2);
    }
}

string getFileName(string buff)
{
    size_t header_end = buff.find("\r\n\r\n");
    if (header_end == string::npos)
        throw HttpExcept(400 ,"Bad Request1");
    size_t start_filename = buff.find ("filename=\"");
    if (start_filename == string::npos)
        throw HttpExcept(400, "Bad Request2");
    start_filename += 10;
    size_t end_filename = buff.find("\r\n");
    if (end_filename == string::npos)
        throw HttpExcept(400, "Bad Request3");   
    return (buff.substr(start_filename , end_filename - start_filename - 1));   
}

bool HttpRequest::openFile(string filename)
{
    bodyFile.open(filename, ios::in | ios::out | ios::binary | ios::trunc);
    if (!bodyFile.is_open())
    {
        cerr << "Fail openning File" << endl;
        lineLocation = END_REQUEST;
        return false;
    }
    return true;
}
void writebody(fstream &bodyFile , string &buffer)
{
    bodyFile.write(buffer.c_str(), buffer.size());
    buffer.clear();
}
void HttpRequest::HandleBoundary() 
{
    contentLength -= buffer.size();
    string start_bound = "--" + Boundary + "\r\n";
    string part_bound = "\r\n--" + Boundary + "\r\n";
    string end_bound = "\r\n--" + Boundary + "--" + "\r\n";
    if (!startBoundFlag && buffer.substr(0, start_bound.size()) == start_bound)
    {
        startBoundFlag = true;
        string filename =  getFileName(buffer.substr(start_bound.size()));
        buffer.erase(0, buffer.find("\r\n\r\n") + 4);
        openFile(filename);
        return writebody(bodyFile, buffer);  
    }
    else if (!startBoundFlag)
        throw HttpExcept(400 ,"Bad Request5");
    size_t posBound = buffer.find("\r\n--");
    if (posBound == string::npos)
        return writebody(bodyFile, buffer);
    else if (buffer.substr(posBound, part_bound.size()) == part_bound)
    {
        if (bodyFile.is_open())
        {
            bodyFile.clear();// check later those 3
            bodyFile.seekg(0, ios::beg);
            bodyFile.close ();
        }
        string filename =  getFileName(buffer.substr(posBound + end_bound.size()));
        buffer.erase(0, buffer.find("\r\n\r\n") + 4);
        openFile(filename);
        return writebody(bodyFile, buffer);  
    }
    else if (buffer.substr(posBound, end_bound.size()) == end_bound)
    {
        buffer.erase(posBound, end_bound.size());
        writebody(bodyFile, buffer); 
    }
    else
    {
        if (posBound)
            bodyFile.write(buffer.substr(0, posBound).c_str(), posBound);
        contentLength += buffer.size() - posBound;
    }
}
void HttpRequest::parseBody()
{
    if (!isChunked && Boundary.empty())
    {
        contentLength -= buffer.size();
        bodyFile.write(buffer.c_str(), buffer.size());
        buffer.clear();
        if (contentLength == 0)
        {
            bodyFile.clear();
            bodyFile.seekg(0, ios::beg);
            lineLocation = END_REQUEST;
        }
    }
    else if (!Boundary.empty())
    {
        HandleBoundary();
        if (contentLength == 0)
        {
            bodyFile.clear();
            bodyFile.seekg(0, ios::beg);
            lineLocation = END_REQUEST;
        }
    }
    else
        HandleChunkedBody();
}
