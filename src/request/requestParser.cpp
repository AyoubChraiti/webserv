#include "../../inc/request.hpp"

void Http::HandleUri() {
    const string allowed = "-._~:/?#[\\]@!$&'()*+,;=%";
    for (size_t i = 0; i < uri.size(); ++i) {
        unsigned char ch = static_cast<unsigned char>(uri[i]);
        if (!isalnum(ch) && allowed.find(ch) == string::npos) {
            throw HttpExcept(400, "Bad Request");
        }
    }

    size_t indexQUERY = uri.find("?");
    if (indexQUERY != string::npos && method == "GET") {
        querystring = uri.substr(indexQUERY + 1);
        uri.erase(indexQUERY);
    }

    string key;
    size_t prevLength = 0;
    bool matched = false;
    for (map<string, routeCnf>::iterator it = conf.routes.begin(); it != conf.routes.end(); ++it) {
        string route = it->first;
        if (uri.size() >= route.size() && uri.compare(0, route.size(), route) == 0) {
            if (back(route) != '/' && uri.size() != route.size() && uri[route.size()] != '/')
                continue;
            if (route.size() > prevLength) {
                prevLength = route.size();
                key = route;
                matched = true;
            }
        }
    }

    if (!matched) {
        throw HttpExcept(404, "No route for path: " + uri);
    }

    mtroute = conf.routes[key];
    string tmpUri = uri;
    tmpUri.erase(0, key.size());
    fullPath = mtroute.alias + tmpUri;
    routeResult = handleRouting(this);
}

void Http::checkIsCGI() {
    if (mtroute.cgi == false || routeResult.autoindex)
        return ;

    isCGI = true;
    if (find(mtroute.cgi_methods.begin(), mtroute.cgi_methods.end(), method) == mtroute.cgi_methods.end())
        throw HttpExcept(405, "Method Not Allowed");
    size_t extensionPos = uri.find(".");
    if (extensionPos == string::npos) {
        throw HttpExcept(404, "Missing file extension for CGI request");
    }
    string extension_uri = uri.substr(extensionPos);
    if (!mtroute.cgi_map.count(extension_uri)) 
        throw HttpExcept(501, "CGI Extension Not Implemented");
    _extensionCGI = mtroute.cgi_map[extension_uri];
}

void countSpace(string &line)
{
    for (size_t i = 0; i < line.size (); i++)
    {
        if (line[i] == ' ' && line[i + 1] == ' ')
            throw HttpExcept(400, "Bad Request");
    }
}

void Http::parseRequestLine ()  {
    size_t index = buffer.find("\r\n");
    if (index == string::npos)
        throw HttpExcept(400, "Bad Request");

    string line = buffer.substr(0, index);
    if (line.size() > MAX_URI_LENGTH)
        throw HttpExcept (414, "URI Too Long");

    string requestLine = trim(line);
    countSpace(requestLine);
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
    if (!configs.empty()) {
        state = READING_HEADERS;
        return ;
    }
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

void Http::ParseHeaders() {
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
        if (headers["Transfer-Encoding"].find("chunked") == string::npos)
            throw HttpExcept(501, "Not Implemented");
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

void Http::checkPost()
{
    state = READING_BODY;
    if (mtroute.fileUpload == false)
        throw HttpExcept(405 ,"Upload Not Supported");
    if (mtroute.uploadStore.empty())
        throw HttpExcept(403, "Missing upload directory");
    buffer.erase(0, 2);
    if (Boundary.empty())
        openFile("file.txt");
}

servcnf chose_srvr_w_host_headr(vector<servcnf>& servers, string& hostHeaderValue) {
    if (hostHeaderValue.empty()) {
        return servers[0];
    }

    for (size_t i = 0; i < servers.size(); i++) {
        for (size_t j = 0; j < servers[i].server_names.size(); j++) {
            if (servers[i].server_names[j] == hostHeaderValue) {
                // cerr << "[DEBUG] match found at server[" << i << "]" << endl;
                // cerr << "maxBody: " << servers[i].maxBodySize << endl;
                return servers[i];
            }
        }
    }
    return servers[0];
}

void Http::HandleHeaders() {
    size_t index;
    while ((index = buffer.find("\r\n")) != string::npos)
    {
        if (index == 0)
            break;
        string headerline = buffer.substr(0, index);
        size_t indexColon = headerline.find(':');
    
        if (indexColon == string::npos)
            throw HttpExcept(400, "Bad Request: Missing colon in header");
        string key = headerline.substr(0, indexColon);
        if (key.empty() || key.find_first_of(" \t") != string::npos)
            throw HttpExcept(400, "Bad Request: Invalid header name");
        string value = trim(headerline.substr(indexColon + 1)); 
        if (value.empty())
            throw HttpExcept(400, "Bad Request: Invalid header value");
        headers.insert(make_pair(trim(key), value));
        buffer.erase(0, index + 2); 
    }
    if (index == string::npos)
        return ;

    ParseHeaders();

    if (configs.size())
        conf = chose_srvr_w_host_headr(configs, host);

    if (!configs.empty()) {
        HandleUri();
        checkIsCGI();
        if (find(mtroute.methodes.begin(), mtroute.methodes.end(), method) == mtroute.methodes.end() && !isCGI)
            throw HttpExcept(405, "Method Not Allowed");
    }

    if (method == "GET" || method == "DELETE")
        state = COMPLETE;
    else
        checkPost();
}

void Http::openFile(string name)
{
    if (name.empty())
        throw HttpExcept(409, "Empty Body");
    string filename = mtroute.uploadStore + name;
    bodyFile.open(filename.c_str(), ios::in | ios::out | ios::binary | ios::trunc);
    if (!bodyFile.is_open())
        throw HttpExcept(500, "Fail openning file");
}

void Http::HandleChunkedBody()
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
            maxBodySizeChunked += contentLength;
            if (maxBodySizeChunked > StringStream(conf.maxBodySize))
            {
                throw HttpExcept(413 , "Request Entity too large Chunked");
                bodyFile.clear();
                bodyFile.close();
            }
            buffer.erase(0, crlf_pos + 2);
        }
        else
            contentLength = remaining;
        if (contentLength == 0)
        {
            bodyFile.clear();
            bodyFile.seekg(0, ios::beg);
            state = COMPLETE;
            hasBody = true;
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

void Http::HandleBoundary() 
{
    if (buffer.size() > contentLength)
        contentLength = 0;
    else {
        contentLength -= buffer.size();
    }
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
        string Towrite = buffer;
        size_t pos_bound = buffer.find("\r\n--");
        if (pos_bound != string::npos) {
            Towrite = buffer.substr(0 , pos_bound);
            buffer.erase(0, pos_bound);
        }
        else
            buffer.clear();
        return writebody(bodyFile, Towrite);  
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
            bodyFile.clear();
            bodyFile.seekg(0, ios::beg);
            bodyFile.close ();
        }
        string filename =  getFileName(buffer.substr(posBound + end_bound.size()));
        buffer.erase(0, buffer.find("\r\n\r\n") + 4);
        openFile(filename);
        string Towrite = buffer;
        size_t pos_bound = buffer.find("\r\n--");
        if (pos_bound != string::npos) {
            Towrite = buffer.substr(0 , pos_bound);
            buffer.erase(0, pos_bound);
        }
        else
            buffer.clear();
        return writebody(bodyFile, Towrite);   
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


void Http::HandleBody()
{
   
    if ((contentLength == 0 && !hasBody)) {
        state = COMPLETE;
        return ;
    }
    hasBody = true;
    if (contentLength > StringStream(conf.maxBodySize))
        throw HttpExcept(413 , "Request Entity too large");
    if (Boundary.empty())
    {
        if (buffer.size() > contentLength || (buffer.find("\r\n\r\n") != string::npos && contentLength > buffer.size()))
            throw HttpExcept(400,  "Bad Request");
        contentLength -= buffer.size();
        writebody(bodyFile,  buffer);
    }
    else
        HandleBoundary();
    if (contentLength == 0) {
        bodyFile.clear();
        bodyFile.seekg(0, ios::beg);
        state = COMPLETE;
    }
}

void Http::parseBody()
{
    if (!isChunked)
        HandleBody();
    else
        HandleChunkedBody();
}
