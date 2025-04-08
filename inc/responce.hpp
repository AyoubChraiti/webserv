#include "request.hpp"

class Response {
private:
    int statusCode;
    string statusText;
    map<string, string> headers;
    unsigned int contentLength;
    string body;
public:
    Response() : statusCode(200),  statusText("OK") {};
    void buildResponse (servcnf& conf, HttpRequest &reqStates, int clientFd);
};