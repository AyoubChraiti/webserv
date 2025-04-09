#include "../../inc/request.hpp"
#include "../../inc/responce.hpp"

RouteResult handleRouting(int fd, HttpRequest& req) {
    RouteResult result = {200, "OK", "", "text/plain", "", false};
    string bestMatch = req.mtroute.root;
    string reqPath = req.uri;
    bool routeFound = false;

    reqPath.erase(reqPath.begin(), reqPath.begin() + bestMatch.size());
    string fullPath = req.mtroute.alias + reqPath;


    cout << "the route: " << req.mtroute.root << endl;
    cout << "the full path: " << fullPath << endl;

    if (isDirectory(fullPath)) {
        if (req.mtroute.index.empty()) {
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
        cout << "file not exist\n";
        throw HttpExcept(404, "Not Found");
    }

    ifstream file(fullPath.c_str(), ios::binary);

    if (!file)
        throw HttpExcept(500, "Internal Server Error");
    
    // stringstream buffer;
    // buffer << file.rdbuf();
    // result.responseBody = buffer.str();
    // file.close();

    result.contentType = getContentType(fullPath);
    return result;
}
