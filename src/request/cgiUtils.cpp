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

bool CGImonitor(int epollFd ,map<int, Http *> &requestmp, map<int, Http *> &pipes_map, map<int, time_t>& timer) 
{
    time_t now = time(NULL);
    for (map<int, time_t>::iterator it = timer.begin(); it != timer.end(); ) {
   
        cout << "here monitor" << endl;
        if (now - it->second > 10) {
            int fd = it->first;
            Http *req = pipes_map[fd];
            cout << "Client " << fd << " timed out\n";
            if (req->cgiPid > 0) {
                kill(req->cgiPid, SIGKILL);
                req->cgiPid = -1;
            }
            if (req->stdoutFd > 0) {
                epoll_ctl(epollFd, EPOLL_CTL_DEL, req->stdoutFd, NULL);
                timer.erase(req->stdoutFd);
                pipes_map.erase(req->stdoutFd);
                close(req->stdoutFd);
                req->stdoutFd = -1;
            }
            if (req->stdinFd > 0) {
                epoll_ctl(epollFd, EPOLL_CTL_DEL, req->stdinFd, NULL);
                timer.erase(req->stdinFd);
                pipes_map.erase(req->stdinFd);
                close(req->stdinFd);
                req->stdinFd = -1;
            }
            if (req->clientFd > 0) {
                epoll_ctl(epollFd, EPOLL_CTL_DEL, req->clientFd, NULL);
                requestmp.erase(req->clientFd);
                close(req->clientFd);
                req->clientFd = -1;
            }
            delete req;
            return true;
        } else {
            ++it;
        }
    }
    return false;
}