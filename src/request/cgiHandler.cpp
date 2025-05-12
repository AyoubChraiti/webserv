#include "../../inc/request.hpp"
#define BUFFER_BYTES 1024

string strUpper(string str)
{
    string res;
    for (size_t i = 0 ;i < str.size(); i++)
    {
        if (str[i] == '-')
            res += "_";
        else
            res += static_cast<char>(toupper(str[i]));
    }
    return res;
}

void setupCGIenv(string &scriptname, HttpRequest &reqStates, vector <char *> &vec, vector<string> &envVar)
{
    map <string, string > env;
    env["GATEWAY_INTERFACE"] = "CGI/1.1";
    env["SERVER_PROTOCOL"] = reqStates.version;
    env["REQUEST_METHOD"] = reqStates.method;
    env["REDIRECT_STATUS"] = "200";
    env["SCRIPT_FILENAME"] = "cgi-bin/script.php"; // edit 
    env["SCRIPT_NAME"] =  scriptname;
    env["QUERY_STRING"] = reqStates.querystring;
    if (reqStates.method == "POST") {
        if (reqStates.headers.find("Content-Type") != reqStates.headers.end())
            env["CONTENT_TYPE"] = reqStates.headers["Content-Type"];
        if (reqStates.headers.find("Content-Length") != reqStates.headers.end())
            env["CONTENT_LENGTH"] = reqStates.headers["Content-Length"];
    }
    for (map<string,string>::iterator it = reqStates.headers.begin(); it != reqStates.headers.end(); it++) {
        if (it->first != "Content-Length" && it->first != "Content-Type")
            env["HTTP_" + strUpper(it->first)] = it->second;
    }
    for (map<string, string>::iterator it = env.begin(); it != env.end(); it++) {
        envVar.push_back(it->first + "=" + it->second);
        vec.push_back(const_cast<char *>(envVar.back().c_str())); 
    }
    vec.push_back(NULL);
}   

void childCGI (HttpRequest &reqStates, int stdoutFd[2],int stdinFd[2], int clientFd)
{
    string path = "./" + reqStates.uri;
    vector <char *> vec;
    vector <string> envVar;
    setupCGIenv(path, reqStates, vec, envVar);
    close (clientFd);
    close(stdoutFd[0]);
    close(stdinFd[1]);
    dup2(stdoutFd[1], STDOUT_FILENO);
    if (reqStates.method == "POST")
        dup2(stdinFd[0], STDIN_FILENO);
    close(stdinFd[0]);
    close(stdoutFd[1]);

    const char *args[] = {path.c_str(), NULL}; // cant change charcter
    execve(path.c_str(), const_cast<char* const*>(args), vec.data()); // cast (cant change string)
    perror("cgi execve failed");
    exit(1);
}

void sigchld_handler(int)
{
    // Reap all finished child processes
    while (waitpid(-1, NULL, WNOHANG) > 0) {}
}

void parseCGIoutput (string &outputCGI)
{
    // size_t pos = outputCGI.find("\r\n\r\n");
    // if (pos == string::npos)
    //     exit(1);
    // cout << "result :" << outputCGI.substr(0, pos) << endl;
}

void handle_cgi_read(int readFd, int epollFd, HttpRequest *reqStates)
{
    char buff[BUFFER_BYTES];
    ssize_t recvBytes = read(readFd, buff, sizeof(buff));
    // cout << "readBytes" << recvBytes <<endl;
    if (recvBytes == -1)
        return (perror("read"), void());
    reqStates->outputCGI.append(buff, recvBytes);
}

void handle_cgi_write(int writeFd, int epollFd,map<int, HttpRequest *> &pipes_map)
{
    HttpRequest *reqStates = pipes_map[writeFd];
    char buff[BUFFER_BYTES];
    reqStates->bodyFile->read(buff, BUFFER_BYTES);
    size_t bytesRead = reqStates->bodyFile->gcount();
    if (bytesRead > 0)
    {
        write(writeFd, buff, bytesRead);
        if (reqStates->bodyFile->eof())
        {
            epoll_ctl(epollFd, EPOLL_CTL_DEL, writeFd, NULL);
            pipes_map.erase(writeFd);
            close(writeFd);
            reqStates->bodyFile->close();
        }
    }
}


int HandleCGI(int epollFd, int clientFd, map<int, HttpRequest> &reqStates, map<int, HttpRequest *> &pipes_map)
{
    map<int, HttpRequest>::iterator it = reqStates.find(clientFd);
    if (it == reqStates.end()) {
        return -1;
    }

    int stdoutFd[2], stdinFd[2];
    if (pipe(stdinFd) == -1 || pipe(stdoutFd) == -1) {
        perror("pipe");
        return -1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return -1;
    }

    if (pid == 0) {
        childCGI(it->second, stdoutFd, stdinFd, clientFd);
    } else {
        close(stdoutFd[1]);
        close(stdinFd[0]);
        if (it->second.method == "POST") {
            add_fds_to_epoll(epollFd, stdinFd[1], EPOLLOUT);
            pipes_map[stdinFd[1]] = &(it->second);
        } else {
            close(stdinFd[1]);
        }
        add_fds_to_epoll(epollFd, stdoutFd[0], EPOLLIN);
        pipes_map[stdoutFd[0]] = &(it->second);
    }
    return 0;
}
