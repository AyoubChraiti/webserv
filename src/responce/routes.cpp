#include "../../inc/request.hpp"
#include "../../inc/responce.hpp"

RouteResult handleRouting(int fd, HttpRequest& req) {
    RouteResult result = {200, "OK", "", "text/plain", "", false, -1, ""};
    string bestMatch = req.mtroute.root;
    string reqPath = req.uri;
    bool routeFound = false;

    reqPath.erase(reqPath.begin(), reqPath.begin() + bestMatch.size());
    string fullPath = req.mtroute.alias + reqPath;

    cout << "alias: " << req.mtroute.alias << endl;

    // cout << "the route: " << req.mtroute.root << endl;
    // cout << "the full path: " << fullPath << endl;

    cout << "index== " << req.mtroute.index << endl;

    if (isDirectory(fullPath)) {
        cout << "its a directory\n";
        if (req.mtroute.index.empty()) {
            cout << "here\n";
            if (req.mtroute.autoindex) {
                result.contentType = "text/html";
                if (reqPath.empty())
                    reqPath = "/";
                result.responseBody = generateAutoIndex(fullPath, reqPath);
                return result;
            }
            throw HttpExcept(403, "Forbiden: no index, no autoindex");
        }
        fullPath += req.mtroute.index;
    }
    
    if (!fileExists(fullPath)) {
        throw HttpExcept(404, "Not Found");
    }

    result.fullPath = fullPath;

    result.resFd = open(fullPath.c_str(), ios::binary);

    if (result.resFd == -1)
        throw HttpExcept(500, "Internal Server Error");

    result.contentType = getContentType(fullPath);
    return result;
}
