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
    // if (reqStates.getMethod() == "GET")
    // {
        string response;
        response += "HTTP/1.1 " + to_string (statusCode) + " " + statusText + "\r\n"; 
        response += "Content-Type: " + headers["Content-Type"] + "\r\n";
        response += "Content-Length: " + headers["Content-Length"] + "\r\n";
        response += "Connection: " + headers["Connection"] + "\r\n";
        response += "\r\n";
        response += body;
        send(clientFd, response.c_str(), response.length(), 0);
        close(clientFd);
    // }
    // else
        // return ;
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
// void handle_http_request(const string& uri) {
//     // Check if this is a CGI request
//     if (uri.find("/cgi-bin/") == 0 && uri.find(".cgi") != string::npos) {
//         size_t query_start = uri.find('?');
//         string script_path = uri.substr(0, query_start);
//         string query_string = (query_start != string::npos) ? 
//                             uri.substr(query_start + 1) : "";
        
//         // Execute the CGI script
//         CgiResult result = execute_cgi(script_path, query_string);
        
//         // Send response to client
//         if (result.status == 0) {
//             // CGI executed successfully - output should include headers
//             cout << result.output;
//         } else {
//             // CGI execution failed
//             cout << "HTTP/1.1 500 Internal Server Error\r\n"
//                  << "Content-Type: text/plain\r\n\r\n"
//                  << "CGI script execution failed";
//         }
//     } else {
//         // Handle non-CGI requests...
//     }
// }


void childCGI (HttpRequest &reqStates, int fds[2])
{
    string scriptPATH = "." + reqStates.getURI();
    close(fds[0]);
    dup2(fds[1], STDOUT_FILENO);
    close(fds[1]);
    vector <string> envcgi;
    envcgi.push_back("REQUEST_METHOD=" + reqStates.getMethod());
    envcgi.push_back("QUERY_STRING="); // mnb3d
    envcgi.push_back("SCRIPT_NAME=" + scriptPATH);
    envcgi.push_back("SERVER_PROTOCOL=HTTP/1.1");
    vector <char *> vec;
    for (vector<string>::iterator it = envcgi.begin(); it != envcgi.end(); it++)
        vec.push_back(const_cast <char*> (it->c_str()));
    vec.push_back(NULL);
    const char *args[] = {scriptPATH.c_str(), NULL};
    execve(scriptPATH.c_str(), const_cast<char* const*>(args), vec.data());
    cerr << "Fail" << endl;
    exit(1);
}
void HandleCGI (HttpRequest &reqStates)
{
    int fds[2];
    if (pipe(fds) == -1)
        exit(1);
    pid_t pid = fork();
    if (pid == -1)
        exit(1);
    if (pid == 0)
        childCGI(reqStates, fds);
    else
        return ;
}
void handle_client_write(int clientFd, int epollFd, mpserv& conf, map<int, HttpRequest>& reqStates) 
{   
    string URI = reqStates[clientFd].getURI();
    if (URI.find("/cgi-bin/") != string::npos)
        HandleCGI(reqStates[clientFd]);
    string host = getInfoClient(clientFd);
    servcnf reqConfig = conf.servers[host];    
    Response response;
    response.buildResponse(reqConfig, reqStates[clientFd], clientFd);
}