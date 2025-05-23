#include "../../inc/request.hpp"

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
void closeFds (int epollFd, map<int, Http *> &requestmp,  Http *req, map<int, Http *> &pipes_map, map<int, time_t>& timer)
{
    if (req->cgiPid > 0) 
    {
        kill(req->cgiPid, SIGKILL);
        waitpid(req->cgiPid, NULL, WNOHANG);
        req->cgiPid = -1;
    }
    if (req->stdoutFd > 0) {
        epoll_ctl(epollFd, EPOLL_CTL_DEL, req->stdoutFd, NULL);
        close(req->stdoutFd);
        timer.erase(req->stdoutFd);
        pipes_map.erase(req->stdoutFd);
        req->stdoutFd = -1;
    }
    if (req->stdinFd > 0) {
        epoll_ctl(epollFd, EPOLL_CTL_DEL, req->stdinFd, NULL);
        close(req->stdinFd);
        timer.erase(req->stdinFd);
        pipes_map.erase(req->stdinFd);
        req->stdinFd = -1;
    }
    if (req->clientFd > 0) {
        
        epoll_ctl(epollFd, EPOLL_CTL_DEL, req->clientFd, NULL);
        close(req->clientFd);
        requestmp.erase(req->clientFd);
        req->clientFd = -1;
    }
    delete req;
    req = NULL;
}
bool CGImonitor(int epollFd ,map<int, Http *> &requestmp, map<int, Http *> &pipes_map, map<int, time_t>& timer) 
{
    time_t now = time(NULL);
    for (map<int, time_t>::iterator it = timer.begin(); it != timer.end(); ) {
        if (now - it->second >= TIMEOUT) {
            int fd = it->first;
            Http *req = pipes_map[fd];
            cout << "Client " << fd << " timed out\n";
            if (req->stateCGI == HEADERS_CGI)
                sendErrorResponse(req->clientFd, 504, "504 Gateway Timeout", req->conf);
            closeFds(epollFd, requestmp,req, pipes_map, timer);
            return true;
        } else {
            ++it;
        }
    }
    return false;
}