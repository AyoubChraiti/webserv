#include "../../inc/responce.hpp"

void Response::buildResponse (servcnf& conf, HttpRequest &reqStates, int clientFd)
{
    ifstream file("index.html");
    stringstream ss;
    ss << file.rdbuf();
    string body = ss.str();
    headers["Content-Type"] = "text/html";
    headers["Content-Length"] = to_string (body.size());
    headers["Connection"] = "close";
    if (reqStates.getMethod() == "GET")
    {
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
    else
        return ;
        // string uri = reqStates.getURI();
        // // if (uri == "/")
        // // {

        // ifstream file(reqStates.getURI());
        // if (!file.is_open())
        // {
        //     statusCode = 404;
        //     statusText = "Not Found";
        // }
        // else
        // {


        // }
}
void handle_client_write(int clientFd, int epollFd, mpserv& conf, map<int, HttpRequest>& reqStates) 
{
    string host = getInfoClient(clientFd);
    servcnf reqConfig = conf.servers[host];    
    Response response;
    response.buildResponse(reqConfig, reqStates[clientFd], clientFd);
}