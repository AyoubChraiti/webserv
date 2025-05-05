#include "../../inc/request.hpp"

HttpRequest::HttpRequest(servcnf config)
{
    lineLocation = REQUEST_LINE;
    isPostKeys = true;
    isChunked = false;
    isCGI = false;
    conf = config;
    remaining = 0;
    querystring = "";
    Boundary = "";
    startBoundFlag= false;
}
HttpRequest::HttpRequest()
{

}
size_t StringStream(const string &string)
{
    size_t num;
    stringstream ss (string);
    ss >> num;
    return num;
}
bool isValidContentLength (const string &value)
{
    return all_of(value.begin(), value.end(), ::isdigit);
}
size_t hexToInt (const string &str)
{
    size_t result;
    stringstream ss (str) ;
    ss >> hex >> result;
    return result;
}
string getFileName(string buff)
{
    size_t header_end = buff.find("\r\n\r\n");
    if (header_end == string::npos)
        throw HttpExcept(400, "Bad Request1");
    size_t start_filename = buff.find ("filename=\"");
    if (start_filename == string::npos)
        throw HttpExcept(400, "Bad Request2");
    start_filename += 10;
    size_t end_filename = buff.find("\r\n");
    if (end_filename == string::npos)
        throw HttpExcept(400, "Bad Request3");   
    return (buff.substr(start_filename , end_filename - start_filename - 1));   
}
void writebody(fstream &bodyFile , string &buffer)
{
    bodyFile.write(buffer.c_str(), buffer.size());
    buffer.clear();
}