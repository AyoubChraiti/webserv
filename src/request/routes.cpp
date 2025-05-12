#include "../../inc/request.hpp"
#include "../../inc/responce.hpp"

RouteResult handleRouting(int fd, HttpRequest* req) {
    RouteResult result = {200, "OK", "", "text/plain", "", false, -1, ""};
    string reqPath = req->uri;

    bool routeFound = false;
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
    
    if (!fileExists(req->fullPath)) {
        cout << "in the routing" << endl;
        throw HttpExcept(404, "Not Found");
    }

    result.fullPath = req->fullPath;

    result.resFd = open(req->fullPath.c_str(), ios::binary);

    if (result.resFd == -1)
        throw HttpExcept(500, "Internal Server Error");

    result.contentType = getContentType(req->fullPath);
    return result;
}
