#include "../../inc/request.hpp"
#include "../../inc/responce.hpp"

RouteResult handleRouting(Http* req) {
    RouteResult result;
    string reqPath = req->uri;


    if (isDirectory(req->fullPath)) {
        if (back(req->fullPath) != '/') {
            result.shouldRDR = true;
            result.redirectLocation = req->uri + "/";
            result.statusCode = 301;
            result.statusText = "Moved Permanently";
            return result;
        }
        if (req->mtroute.index.empty()) {
            if (req->mtroute.autoindex) {
                result.autoindex = true;
                result.contentType = "text/html"; 
                if (reqPath.empty())
                    reqPath = "/";
                result.responseBody = generateAutoIndex(req->fullPath, reqPath);
                return result;
            }
            throw HttpExcept(403, "Forbiden: no index, no autoindex");
        }
        req->fullPath += req->mtroute.index;
    }
    cout << "File : " << req->fullPath << endl;
    
    if (!fileExists(req->fullPath)) {
        throw HttpExcept(404, "Not Found");
    }
    if (req->mtroute.cgi || req->method == "POST")
        return result;

    ifstream* file = new ifstream(req->fullPath.c_str(), ios::binary);
    if (!file->is_open()) {
        delete file;
        throw HttpExcept(500, "Internal Server Error");
    }
    result.fileStream = file;
    result.contentType = getContentType(req->fullPath);
    return result;
}
