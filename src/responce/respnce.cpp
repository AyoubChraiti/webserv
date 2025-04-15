#include "../../inc/responce.hpp"

void sendRedirect(int fd, const string& location, HttpRequest& req) {
    stringstream response;

    response << "HTTP/1.1 301 Moved Permanently\r\n";
    response << "Location: " << location << "\r\n";
    response << "Content-Type: text/html\r\n";
    response << "Content-Length: 0\r\n";
    response << "Connection: " << req.connection << "\r\n";
    response << "\r\n";

    string responseStr = response.str();
    send(fd, responseStr.c_str(), responseStr.size(), 0);
}

size_t getContentLength(const string& path) {
    struct stat fileStat;
    if (stat(path.c_str(), &fileStat) == -1) {
        perror("stat");
        return 0;
    }
    return fileStat.st_size;
}

int sendFileInChunks(int clientFd, std::ifstream& fileStream, off_t fileSize) {
    char buffer[BUFFER_SIZE];
    off_t bytesSent = 0;

    while (bytesSent < fileSize) {
        // Read a chunk of the file
        fileStream.read(buffer, BUFFER_SIZE);
        size_t bytesRead = fileStream.gcount();

        if (bytesRead == 0)
            break;

        std::stringstream chunkHeader;
        chunkHeader << std::hex << bytesRead << "\r\n";  // Hexadecimal size of the chunk

        std::string chunkHeaderStr = chunkHeader.str();
        if (send(clientFd, chunkHeaderStr.c_str(), chunkHeaderStr.size(), 0) == -1) {
            perror("send chunk size header");
            return -1;
        }

        if (send(clientFd, buffer, bytesRead, 0) == -1) {
            perror("send chunk data");
            return -1;
        }

        const char* crlf = "\r\n";
        if (send(clientFd, crlf, 2, 0) == -1) {
            perror("send CRLF after chunk");
            return -1;
        }

        bytesSent += bytesRead;
    }

    const char* finalChunk = "0\r\n\r\n";
    if (send(clientFd, finalChunk, 5, 0) == -1) {
        perror("send final chunk");
        return -1;
    }
    return 0;  // Success
}

int getMethode(int clientFd, int epollFd, HttpRequest& req, map<int, HttpRequest>& requestmp) {
    RouteResult routeResult = handleRouting(clientFd, req);

    stringstream response;
    response << "HTTP/1.1 " << routeResult.statusCode << " " << routeResult.statusText << "\r\n";
    response << "Content-Type: " << routeResult.contentType << "\r\n";

    if (routeResult.resFd != -1) {
        response << "Content-Length: " << getContentLength(routeResult.fullPath) << "\r\n";
        // response << "Accept-Ranges: bytes\r\n";
    } else {
        response << "Content-Length: " << routeResult.responseBody.size() << "\r\n";
    }

    response << "Connection: " << "close" << "\r\n";
    response << "\r\n";

    string headerStr = response.str();
    if (send(clientFd, headerStr.c_str(), headerStr.size(), 0) == -1) {
        perror("send headers");
        return -1;
    }

    if (routeResult.resFd != -1) {
        char buffer[BUFFER_SIZE];

        while (true) {
            size_t bytesRead = read(routeResult.resFd, buffer, BUFFER_SIZE);

            if (bytesRead <= 0)
                break;

            streamsize totalSent = 0;
            while (totalSent < bytesRead) {
                ssize_t sent = send(clientFd, buffer + totalSent, bytesRead - totalSent, 0);
                if (sent == -1) {
                    if (errno == EPIPE || errno == ECONNRESET) {
                        cout << "Socket is not ready for writing, try again later" << endl;
                        break; // Non-blocking mode, try again later
                    }
                    perror("send body (file)");
                    close(routeResult.resFd);
                    return -1;
                }
                totalSent += sent;
            }
        }
        close(routeResult.resFd);
    }
    else {
        const string& body = routeResult.responseBody;
        ssize_t totalSent = 0;
        while (totalSent < (ssize_t)body.size()) {
            ssize_t sent = send(clientFd, body.c_str() + totalSent, body.size() - totalSent, 0);
            if (sent == -1) {
                perror("int send body (string)");
                return -1;
            }
            totalSent += sent;
        }
    }

    return 0;
}

// void handle_client_write(int fd, int epollFd, mpserv& conf, map<int, HttpRequest>& requestmp) {
//     HttpRequest req = requestmp[fd];

//     try {
//         if (!req.mtroute.redirect.empty()) {
//             sendRedirect(fd, req.mtroute.redirect, req);
//             closeOrSwitch(fd, epollFd, req, requestmp);
//             return;
//         }
//         if (req.method == "GET") {
//             getMethode(fd, epollFd, req, requestmp);
//             closeOrSwitch(fd, epollFd, req, requestmp);
//             return;
//         }
//     }
//     catch (const HttpExcept& e) {
//         sendErrorResponse(fd, e.getStatusCode(), e.what(), req.conf);
//         requestmp.erase(fd);
//         struct epoll_event ev;
//         ev.data.fd = fd;
//         if (epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, &ev) == -1)
//             cout << "epoll ctl error in the client write\n";
//         close(fd);
//         return;
//     }
// }








// STARTER




void Response::buildResponse (servcnf& conf, HttpRequest &reqStates, int clientFd)
{
    ifstream file("index.html");
    stringstream ss;
    ss << file.rdbuf();
    string body = ss.str();
    headers["Content-Type"] = "text/html";
    headers["Content-Length"] = to_string (body.size());
    headers["Connection"] = "close";
    string response;
    response += "HTTP/1.1 " + to_string (statusCode) + " " + statusText + "\r\n"; 
    response += "Content-Type: " + headers["Content-Type"] + "\r\n";
    response += "Content-Length: " + headers["Content-Length"] + "\r\n";
    response += "Connection: " + headers["Connection"] + "\r\n";
    response += "\r\n";
    response += body;
    send(clientFd, response.c_str(), response.length(), 0);
    close(clientFd);
}


void setupCGIenv(string &scriptPATH, string &QUERYstring , HttpRequest reqStates, vector <char *> &vec)
{
    vector <string> envcgi;
    envcgi.push_back("GATEWAY_INTERFACE=CGI/1.1");
    envcgi.push_back("SERVER_PROTOCOL=HTTP/1.1");
    envcgi.push_back("REQUEST_METHOD=" + reqStates.method);
    envcgi.push_back("QUERY_STRING=" + QUERYstring);
    envcgi.push_back("SCRIPT_NAME=" + scriptPATH);
    for (vector<string>::iterator it = envcgi.begin(); it != envcgi.end(); it++)
        vec.push_back(const_cast <char*> (it->c_str())); 
    vec.push_back(NULL);
}

void childCGI (HttpRequest &reqStates, int fds[2], int clientFd)
{
    size_t indexQUERY = reqStates.uri.find("?");
    string scriptPATH = "." + reqStates.uri.substr(0, indexQUERY);
    if (access(scriptPATH.c_str(), X_OK) != 0)
        throw HttpExcept(404, "Not Found");
    string QUERYstring = (indexQUERY != string::npos) ? reqStates.uri.substr(indexQUERY + 1) : "";
    vector <char *> vec;
    setupCGIenv(scriptPATH, QUERYstring, reqStates, vec);

    close(fds[0]);
    close (clientFd);
    dup2(fds[1], STDOUT_FILENO);
    close(fds[1]);

    const char *args[] = {scriptPATH.c_str(), NULL}; // cant change charcter
    execve(scriptPATH.c_str(), const_cast<char* const*>(args), vec.data()); // cast (cant change string)
    cerr << "Fail" << endl;
    exit(1);
}

int HandleCGI (int clientFd, HttpRequest &reqStates)
{
    int fds[2];
    if (pipe(fds) == -1)
    {
        perror("pipe"); 
        return -1; 
    }
    pid_t pid = fork();
    if (pid == -1)
    {
        perror("fork");
        return -1;
    }
    if (pid == 0)
        childCGI(reqStates, fds, clientFd);
    else
    {
        close (fds[1]);
        string output_cgi;
        char buff[1024];
        while (ssize_t recvBytes = read(fds[0], buff, sizeof(buff)))
        {
            if (recvBytes == -1)
            {
                perror("read");
                return -1;
            }
            output_cgi.append(buff, recvBytes);
        }
        close(fds[0]);
        int status;
        waitpid(pid, &status, 0);
        if (WEXITSTATUS(status) != 0) 
        {
            std::cerr << "CGI process failed" << std::endl;
            return -1;
        }
        string starterline = "HTTP/1.1, 200 OK\r\n";
        send(clientFd, starterline.c_str(),  starterline.length(), 0);
        send(clientFd, output_cgi.c_str(),  output_cgi.length(), 0);
        close(clientFd);
    }
    return 0;
}

void handle_client_write(int clientFd, int epollFd, mpserv& conf, map<int, HttpRequest>& requestmp) 
{   
    try
    {
        string URI = requestmp[clientFd].uri;
        if (URI.find("/cgi-bin/") != string::npos)
        {
            if (HandleCGI(clientFd, requestmp[clientFd]) == -1)
                return ;
        }
        else
        {
            string host = getInfoClient(clientFd);
            servcnf reqConfig = conf.servers[host];    
            Response response;
            response.buildResponse(reqConfig, requestmp[clientFd], clientFd);
        }
    }
    catch(const HttpExcept& e)
    {
        sendErrorResponse(clientFd, e.getStatusCode(), e.what(), requestmp[clientFd].conf);
        requestmp.erase(clientFd);
        struct epoll_event ev;
        ev.data.fd = clientFd;
        if (epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, &ev) == -1)
            cout << "epoll ctl error in the client write\n";
        close(clientFd);
        return;
    }
    requestmp.erase(clientFd);
}
