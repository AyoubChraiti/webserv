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
    env["SCRIPT_NAME"] = scriptname;
    env["QUERY_STRING"] = reqStates.querystring;
    if (reqStates.method == "POST") 
    {
        if (reqStates.headers.find("Content-Type") != reqStates.headers.end())
            env["CONTENT_TYPE"] = reqStates.headers["Content-Type"];
        if (reqStates.headers.find("Content-Length") != reqStates.headers.end())
            env["CONTENT_LENGTH"] = reqStates.headers["Content-Length"];
    }
    for (map<string,string>::iterator it = reqStates.headers.begin(); it != reqStates.headers.end(); it++)
    {
        if (it->first != "Content-Length" && it->first != "Content-Type")
            env["HTTP_" + strUpper(it->first)] = it->second;
    }
    for (map<string, string>::iterator it = env.begin(); it != env.end(); it++)
    {
        envVar.push_back(it->first + "=" + it->second);
        vec.push_back(const_cast<char *>(envVar.back().c_str())); 
    }
    vec.push_back(NULL);
}   

void childCGI (HttpRequest &reqStates, int stdoutFd[2],int stdinFd[2], int clientFd)
{
    string path = "." + reqStates.uri;
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

// int HandleCGI (int epollFd, int clientFd, HttpRequest &reqStates)
// {
//     int stdoutFd[2], stdinFd[2];
//     if (pipe(stdinFd) == -1 || pipe(stdoutFd) == -1)
//         return (perror("pipe"), -1); 
//     pid_t pid = fork();
//     if (pid == -1)
//         return (perror("fork"), -1);
//     if (pid == 0)
//         childCGI(reqStates, stdoutFd, stdinFd, clientFd);
//     else
//     {
//         close (stdoutFd[1]);
//         close(stdinFd[0]);
//         char buff[BUFFER_BYTES];
//         if (reqStates.method == "POST")
//         {
//             while (reqStates.bodyFile.read(buff, BUFFER_BYTES))
//             {
//                 size_t bytesRead = reqStates.bodyFile.gcount();
//                 write(stdinFd[1], buff, bytesRead);
//             }
//             if (reqStates.bodyFile.gcount() > 0)
//                 write(stdinFd[1], buff, reqStates.bodyFile.gcount());
//         }
//         reqStates.bodyFile.close();
//         close(stdinFd[1]);
//         while (ssize_t recvBytes = read(stdoutFd[0], buff, sizeof(buff)))
//         {
//             if (recvBytes == -1)
//                 return (perror("read"), -1);
//             reqStates.outputCGI.append(buff, recvBytes);
//         }
//         close(stdoutFd[0]);
//         int status;
//         waitpid(pid, &status, 0);
//         if (WEXITSTATUS(status) != 0) 
//         {
//             std::cerr << "CGI process failed" << std::endl;
//             return -1;
//         }
//     }
    // return 0;
// }

int HandleCGI (int epollFd ,int clientFd, map<int, HttpRequest> &reqStates, map<int, HttpRequest *> &pipes_map)
{
    int stdoutFd[2], stdinFd[2];
    if (pipe(stdinFd) == -1 || pipe(stdoutFd) == -1)
        return (perror("pipe"), -1); 

    pid_t pid = fork();
    if (pid == -1)
        return (perror("fork"), -1);
    if (pid == 0)
        childCGI(reqStates[clientFd], stdoutFd, stdinFd,clientFd);

    else
    {
        close (stdoutFd[1]);
        close(stdinFd[0]);
        if (reqStates[clientFd].method == "POST")
            add_fds_to_epoll(epollFd, stdinFd[1], EPOLLOUT); // write on cgi
        add_fds_to_epoll(epollFd, stdoutFd[0], EPOLLIN); // read cgi  
        pipes_map[stdinFd[1]] = &reqStates[clientFd]; 
        pipes_map[stdoutFd[0]] = &reqStates[clientFd];
    }
    return 0;
}