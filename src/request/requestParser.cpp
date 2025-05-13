#include "../../inc/request.hpp"

void HttpRequest::HandleUri()
{
    const string allowed = "-._~:/?#[\\]@!$&'()*+,;=";
    for (size_t i = 0; i < uri.size(); ++i) {
        unsigned char ch = static_cast<unsigned char>(uri[i]);
        if (!isalnum(ch) && allowed.find(ch) == string::npos)
            throw HttpExcept(400, "Bad Request");
    }
    size_t indexQUERY = uri.find("?");
    if (indexQUERY != string::npos && method == "GET") {
        querystring =  uri.substr(indexQUERY + 1);
        uri.erase(indexQUERY);
    }
    map <string , routeCnf>::iterator it = conf.routes.begin();
    size_t prevLength = 0;
    string key;
    bool flag = false; 
    for (; it != conf.routes.end(); it++)
    {
        string route = it->first;
        if (uri.size() >= route.size() && !uri.compare(0, route.size(), route)) 
        {
            if (back(route) != '/' && uri.size() != route.size() && uri[route.size()] != '/')
                continue;
            if (route.size() > prevLength) {
                prevLength = route.size();
                key = route;
                flag = true;
            }
        }
    }
    if (!flag)
        throw HttpExcept(404, "No route for path: " + uri);
    string tmpUri = uri;
    mtroute = conf.routes[key];
    string bestMatch = mtroute.root;
    tmpUri.erase(tmpUri.begin(), tmpUri.begin() + bestMatch.size());
    fullPath = mtroute.alias + tmpUri;
}
void HttpRequest::checkIsCGI()
{
    if (mtroute.cgi_map.empty() || mtroute.cgi == false)
        return ;
    if (mtroute.cgi) {
        cout << mtroute.cgi_map[".py"] << endl;
    }
    isCGI = true;
    if (find(mtroute.cgi_methods.begin(), mtroute.cgi_methods.end(), method) == mtroute.cgi_methods.end())
        throw HttpExcept(405, "Method Not Allowed");
    size_t extensionPos = uri.find(".");
    if (extensionPos == string::npos)
        return ;
    string extension_uri = uri.substr(extensionPos);
    if (!mtroute.cgi_map.count(extension_uri))
        throw HttpExcept(501, "CGI Extension Not Implemented");
    _extensionCGI = mtroute.cgi_map[extension_uri];
}
void HttpRequest::parseRequestLine () 
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
    if (method != "GET" && method != "POST" && method != "DELETE") {
        cout << '1' << endl;
        throw HttpExcept(501, "Not Implemented");
    }
    if (version != "HTTP/1.1")
        throw HttpExcept(505, "HTTP Version Not Supported");
    buffer.erase(0, index + 2);
    HandleUri();
    checkIsCGI();
    if (find(mtroute.methodes.begin(), mtroute.methodes.end(), method) == mtroute.methodes.end() && !isCGI)
        throw HttpExcept(405, "Method Not Allowed");
    state = READING_HEADERS;
}

bool isValidHostHeader(const string& host) {
    size_t colonPos = host.find(':');
    string hostname = (colonPos == string::npos) ? host : host.substr(0, colonPos);
    string port = (colonPos == string::npos) ? "" : host.substr(colonPos + 1);


    for (size_t i = 0; i < hostname.length(); ++i) {
        char c = hostname[i];
        if (!isalnum(c) && c != '.' && c != '-') {
            return false;
        }
    }
    if (!port.empty()) {
        for (size_t i = 0; i < port.length(); ++i) {
            if (!isdigit(port[i])) {
                return false;
            }
        }
    }
    return true;
}

void HttpRequest::ParseHeaders()
{
    if (!headers.count("Host") || !isValidHostHeader(headers["Host"])) // edit func
        throw HttpExcept(400, "Bad Request");
    host = headers["Host"];

    if (headers.count("Content-Type") > 0)
    {
        size_t pos = headers["Content-Type"].find("boundary=");
        if (pos != string::npos) {
            Boundary = headers["Content-Type"].substr(pos + 9);
        }
    }
    else if (method == "POST")
        throw HttpExcept(400, "Bad Request");
    
    if (headers.count("Transfer-Encoding") > 0)
    {
        if (headers["Transfer-Encoding"].find("Chunked") == string::npos) {
            cout << "2" << endl;
            throw HttpExcept(501, "Not Implemented");
        }
        isChunked = true; 
    }  
    else if (headers.count("Content-Length") > 0 && isValidContentLength(headers["Content-Length"]))
        contentLength = StringStream(headers["Content-Length"]);
    else if (method == "POST")
        isPostKeys = false;

    if (!headers.count("Connection") || (headers["Connection"] != "keep-alive" && headers["Connection"] != "close"))
        throw HttpExcept(400 ,"Bad Request");
    connection = headers["Connection"];

    if (method == "POST" && !isPostKeys)
        throw HttpExcept(400 ,"Bad Request");
}
void HttpRequest::checkPost()// heere
{
    state = READING_BODY;
    if (mtroute.fileUpload == false)
        throw HttpExcept(405 ,"Upload Not Supported"); 
    buffer.erase(0, 2);
    if (Boundary.empty())
        openFile("bigfile.txt"); // edit to tmp after 
    
}
void HttpRequest::HandleHeaders()
{
    size_t index;
    while ((index = buffer.find("\r\n")) != string::npos)
    {
        if (index == 0)
            break;
        string headerline = buffer.substr(0, index);
        size_t indexColon;
        if ((indexColon = headerline.find(':')) == string::npos)
            throw HttpExcept(400 ,"Bad Request");
        string key = trim(headerline.substr(0, indexColon)); // check later
        string value = trim(headerline.substr(indexColon + 1)); // check later
        headers.insert(make_pair(key, value));
        buffer.erase(0, index + 2);
    }
    if (index == string::npos)
        return ;
    ParseHeaders();
    if (method == "GET" || method == "DELETE")
        state = COMPLETE;
    else
        checkPost();
}

void HttpRequest::HandleChunkedBody()
{
    while (!buffer.empty()) 
    {
        size_t crlf_pos;
        if (remaining == 0)
        {
            if (buffer.substr(0,2) == "\r\n")
                buffer.erase(0, 2); 
            crlf_pos = buffer.find("\r\n");
            if (crlf_pos == string::npos) 
                return ;
            contentLength = hexToInt(buffer.substr(0, crlf_pos));
            buffer.erase(0, crlf_pos + 2);
        }
        else
            contentLength = remaining;
        if (contentLength == 0)
        {
            bodyFile.clear();
            bodyFile.seekg(0, ios::beg);
            state = COMPLETE;
            return;
        }
        size_t bytesTowrite = min(contentLength, buffer.size());
        bodyFile.write(buffer.c_str(), bytesTowrite);
        (contentLength > buffer.size()) ? buffer.erase(0, buffer.size()) : buffer.erase(0, contentLength);
        remaining = contentLength - bytesTowrite;
        if (!remaining && buffer.size() >= 2 && buffer.find("\r\n") == string::npos)
            throw HttpExcept(400 ,"Bad Request");
    }
}

bool HttpRequest::openFile(string filename)
{
    bodyFile.open(filename.c_str(), ios::in | ios::out | ios::binary | ios::trunc);
    if (!bodyFile.is_open())
    {
        cerr << "Fail openning File" << endl;
        state = COMPLETE;
        return false;
    }
    return true;
}

void HttpRequest::HandleBoundary() 
{
    contentLength -= buffer.size();
    string start_bound = "--" + Boundary + "\r\n";
    string part_bound = "\r\n--" + Boundary + "\r\n";
    string end_bound = "\r\n--" + Boundary + "--" + "\r\n";
    if (!startBoundFlag)
    {
        if (buffer.find("\r\n\r\n") == string::npos) {
            if (contentLength == 0)
                throw HttpExcept(400, "Bad Request");
            contentLength += buffer.size();
            return ;
        }
        startBoundFlag = true;  
        string filename =  getFileName(buffer.substr(start_bound.size()));
        buffer.erase(0, buffer.find("\r\n\r\n") + 4);
        openFile(filename);
        return writebody(bodyFile, buffer);  
    }
    size_t posBound = buffer.find("\r\n--");
    if (posBound == string::npos)
        return writebody(bodyFile, buffer);
    else if (buffer.substr(posBound, part_bound.size()) == part_bound)
    {
        if (buffer.find("\r\n\r\n") == string::npos) {
            if (contentLength == 0)
                throw HttpExcept(400, "Bad Request");
            contentLength += buffer.size();
            return ;
        }
        if (bodyFile.is_open())
        {
            bodyFile.clear(); // check later those 3
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
        return writebody(bodyFile, buffer); 
    }
    else
    {
        if (posBound)
            bodyFile.write(buffer.substr(0, posBound).c_str(), posBound);
        contentLength += (buffer.size() - posBound);
    }
}

void HttpRequest::parseBody()
{
    if (!isChunked)
    {
        if (contentLength > StringStream(conf.maxBodySize))
            throw HttpExcept(413 , "Request Entity too large");
        if (Boundary.empty()) 
        {
            contentLength -= buffer.size();
            writebody(bodyFile,  buffer);
        }
        else
            HandleBoundary();
        if (contentLength == 0) {
            cout << "End reading File" << endl;
            bodyFile.clear();
            bodyFile.seekg(0, ios::beg);
            state = COMPLETE;
        }
    }
    else
        HandleChunkedBody();
}
