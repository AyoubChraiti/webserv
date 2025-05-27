#include "../../inc/request.hpp"

void setupCGIenv(string &FullPath, Http *reqStates, vector <const char *> &vec, vector<string> &envVar) {
    map <string, string > env;
    env["GATEWAY_INTERFACE"] = "CGI/1.1";
    env["SERVER_PROTOCOL"] = reqStates->version;
    env["REQUEST_METHOD"] = reqStates->method;
    env["REDIRECT_STATUS"] = "200";
    env["SCRIPT_FILENAME"] = FullPath;
    env["SCRIPT_NAME"] = FullPath.substr(FullPath.find_last_of("/") + 1);
    env["QUERY_STRING"] = reqStates->querystring;
    if (reqStates->method == "POST") {
        if (reqStates->headers.find("Content-Type") != reqStates->headers.end())
            env["CONTENT_TYPE"] = reqStates->headers["Content-Type"];
        if (reqStates->headers.find("Content-Length") != reqStates->headers.end())
            env["CONTENT_LENGTH"] = reqStates->headers["Content-Length"];
    }
    for (map<string,string>::iterator it = reqStates->headers.begin(); it != reqStates->headers.end(); it++) {
        if (it->first != "Content-Length" && it->first != "Content-Type")
            env["HTTP_" + strUpper(it->first)] = it->second;
    }
    envVar.reserve(env.size());
    for (map<string, string>::iterator it = env.begin(); it != env.end(); it++) {
        envVar.push_back(it->first + "=" + it->second);
        vec.push_back(const_cast<char *>(envVar.back().c_str())); 
    }
    vec.push_back(NULL);
}

void childCGI (Http *reqStates, int stdoutFd[2],int stdinFd[2], int clientFd)
{
    vector <const char *> vec;
    vector <string> envVar;
    setupCGIenv(reqStates->fullPath, reqStates, vec, envVar);
    close (clientFd);
    close(stdoutFd[0]);
    close(stdinFd[1]);
    dup2(stdoutFd[1], STDOUT_FILENO);
    dup2(stdoutFd[1], STDERR_FILENO);
    if (reqStates->method == "POST")
        dup2(stdinFd[0], STDIN_FILENO);
    else
        close(STDIN_FILENO);
    close(stdinFd[0]);
    close(stdoutFd[1]);

    const char *args[] = {reqStates->_extensionCGI.c_str(), reqStates->fullPath.c_str(), NULL}; 
    execve(reqStates->_extensionCGI.c_str(), const_cast<char* const*>(args), const_cast<char* const*>(vec.data())); 
    perror("cgi execve failed");
    exit(1);
}

void sigchld_handler(int)
{
    while (waitpid(-1, NULL, WNOHANG) > 0) {}
}

void handle_cgi_read(int epollFd, int readFd, Http *reqStates, map<int, Http *> &pipes_map) {
    char buff[BUFFER_SIZE];
    ssize_t recvBytes = read(readFd, buff, sizeof(buff));
    if (recvBytes == -1)
        throw HttpExcept(500, "error while reading");
    reqStates->outputCGI.append(buff, recvBytes);
    modifyState(epollFd, pipes_map[readFd]->clientFd, EPOLLOUT);
}

void handle_cgi_write(int writeFd, int epollFd, map<int, Http *> &pipes_map, map<int, time_t> &timer) {
    Http *reqStates = pipes_map[writeFd];
    char buff[BUFFER_SIZE];
    reqStates->bodyFile.read(buff, BUFFER_SIZE);
    size_t bytesRead = reqStates->bodyFile.gcount();
    if (bytesRead > 0)
    {
        if (write(writeFd, buff, bytesRead) == -1)
            throw HttpExcept(500, "eroor while writing to a fd");
        if (reqStates->bodyFile.eof())
        {
            timer.erase(writeFd);
            pipes_map[writeFd]->stdinFd = -1;
            epoll_ctl(epollFd, EPOLL_CTL_DEL, writeFd, NULL);
            pipes_map.erase(writeFd);
            close(writeFd);
            reqStates->bodyFile.close();
        }
    }
}


int HandleCGI(int epollFd, int clientFd, map<int, Http *> &reqStates, map<int, Http *> &pipes_map, map<int ,time_t > &timer) {
    time_t now = time(NULL);

    map<int, Http *>::iterator it = reqStates.find(clientFd);
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
    }
    else {
        reqStates[clientFd]->cgiPid = pid;
        close(stdoutFd[1]);
        close(stdinFd[0]);
        if (it->second->method == "POST") {
            add_fds_to_epoll(epollFd, stdinFd[1], EPOLLOUT);
            pipes_map[stdinFd[1]] = it->second;
            timer[stdinFd[1]] = now;
            pipes_map[stdinFd[1]]->stdinFd = stdinFd[1];
        }
        else
            close(stdinFd[1]);
        add_fds_to_epoll(epollFd, stdoutFd[0], EPOLLIN);
        pipes_map[stdoutFd[0]] = it->second;
        timer[stdoutFd[0]] = now;
        pipes_map[stdoutFd[0]]->stdoutFd = stdoutFd[0];
    }
    return 0;
}

