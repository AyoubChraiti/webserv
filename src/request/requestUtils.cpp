#include "../../inc/request.hpp"

HttpRequest::HttpRequest(servcnf config)
{
    lineLocation = REQUEST_LINE;
    isPostKeys = false;
    isChunked = false;
    isCGI = false;
    conf = config;
    remaining = 0;
    querystring = "";
}
HttpRequest::HttpRequest()
{

}