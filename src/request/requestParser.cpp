#include "../../inc/request.hpp"


void HttpRequest::HandleUri()
{
    const std::string allowed = "-._~:/?#[\\]@!$&'()*+,;=";
    for (size_t i = 0; i < uri.size(); ++i) {
        unsigned char ch = static_cast<unsigned char>(uri[i]);
        if (!std::isalnum(ch) && allowed.find(ch) == std::string::npos)
            throw HttpExcept(400, "Bad Request");
    }
    size_t indexQUERY = uri.find("?");
    if (indexQUERY != string::npos && method == "GET")
    {
        querystring =  uri.substr(indexQUERY + 1);
        uri.erase(indexQUERY);
    }
    map <string , routeCnf>::iterator it = conf.routes.begin();
    size_t prevLength = 0;
    string key;
    bool flag = false; 
    for (; it != conf.routes.end(); it++)
    {
        const string route = it->first;
        if (uri.size() >= route.size() && !uri.compare(0, route.size(), route)) 
        {
            if (route.back() != '/' && uri.size() != route.size() && uri[route.size()] != '/')
                continue;
            if (route.size() > prevLength)
            {
                prevLength = route.size();
                key = route;
                flag = true;
            }
        }
    }
    if (!flag)
        throw HttpExcept(404, "No route for path: " + uri);
    uri.erase(0, key.size());
    uri = conf.routes[key].alias + uri;
    // if (!is_file(uri))
        // cerr << "File Not Found" << endl; If path doesn’t exist → return 404 Not Found // fix this later
    mtroute = conf.routes[key]; 
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

bool isValidHostHeader(const string& host) {
    size_t colonPos = host.find(':');
    string hostname = (colonPos == string::npos) ? host : host.substr(0, colonPos);
    string port = (colonPos == string::npos) ? "" : host.substr(colonPos + 1);

    // Validate hostname (basic check for alphanumeric and dots)
    for (char c : hostname) {
        if (!isalnum(c) && c != '.' && c != '-') {
            return false;
        }
    }

    // Validate port (if present)
    if (!port.empty() && !all_of(port.begin(), port.end(), ::isdigit)) {
        return false;
    }

    return true;
}

void HttpRequest::ParseHeaders()
{
    // Validate Host
    if (!headers.count("Host") || !isValidHostHeader(headers["Host"])) // edit func
        throw HttpExcept(400, "Bad Request");
    host = headers["Host"];

    // Validate Content-Type 
    if (headers.count("Content-Type") > 0)
    {
        size_t pos = headers["Content-Type"].find("boundary=");
        if (pos != string::npos)
            Boundary = headers["Content-Type"].substr(pos + 9);
    }
    else if (method == "POST")
        throw HttpExcept(400, "Bad Request");
    
    // Validate Content-Length or Transfer-Encoding for POST
    if (headers.count("Transfer-Encoding") > 0)
    {
        if (headers["Transfer-Encoding"].find("Chunked") == string::npos)
            throw HttpExcept(501, "Not Implemented");
        isChunked = true; 
    }  
    else if (headers.count("Content-Length") > 0 && isValidContentLength(headers["Content-Length"]))
        contentLength = StringStream(headers["Content-Length"]);
    else if (method == "POST")
        isPostKeys = false;

    // Validate connection
    if (!headers.count("Connection") || (headers["Connection"] != "keep-alive" && headers["Connection"] != "close"))
        throw HttpExcept(400 ,"Bad Request");
    connection = headers["Connection"];

    if (method == "POST" && !isPostKeys)
        throw HttpExcept(400 ,"Bad Request");
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
        lineLocation = END_REQUEST;
    else
    {
        lineLocation = BODY;
        buffer.erase(0, 2);
        if (Boundary.empty())
            openFile("bigfile.txt"); // edit to tmp after 
    }
}

void HttpRequest::HandleChunkedBody()
{
    // check errors \r\n
    while (!buffer.empty()) 
    {
        size_t crlf_pos;
        if (remaining == 0)
        {
            if (buffer.substr(0,2) == "\r\n")
                buffer.erase(0, 2); 
            crlf_pos = buffer.find("\r\n");
            if (crlf_pos == std::string::npos) 
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
            lineLocation = END_REQUEST;
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
    bodyFile.open(filename, ios::in | ios::out | ios::binary | ios::trunc);
    if (!bodyFile.is_open())
    {
        cerr << "Fail openning File" << endl;
        lineLocation = END_REQUEST;
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
            lineLocation = END_REQUEST;
        }
    }
    else
        HandleChunkedBody();
}
