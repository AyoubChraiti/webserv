#include "../../inc/request.hpp"
#include "../../inc/responce.hpp"

RouteResult handleRouting(Http* req) {
    RouteResult result = {200, "OK", "", "text/plain", "", false, false};
    string reqPath = req->uri;

    cout << "the full_path : " << req->fullPath << endl;

    if (isDirectory(req->fullPath)) {
        cout << "is a directory <-------------------" << endl;
        if (back(req->fullPath) != '/') {
            cout << "code for rederection <-------------------" << endl;
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
    if (!fileExists(req->fullPath)) {
        throw HttpExcept(404, "Not Found");
    }

    ifstream* file = new ifstream(req->fullPath.c_str(), ios::binary);
    if (!file->is_open()) {
        delete file;
        throw HttpExcept(500, "Internal Server Error");
    }
    result.fileStream = file;
    result.contentType = getContentType(req->fullPath);
    return result;
}
