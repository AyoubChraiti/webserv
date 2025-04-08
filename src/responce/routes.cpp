#include "../../inc/request.hpp"
#include "../../inc/responce.hpp"

// Define a directory or a file from where the file should be searched (for example,
// if url /kapouet is aliased to /tmp/www, url /kapouet/pouic/toto/pouet is
// /tmp/www/pouic/toto/pouet).

/*  the uri -> /kapouet 
    the req uri -> /kapouet/pouic/toto/pouet
    the final path -> www/html/pouic/toto/pouet
*/

RouteResult handleRouting(HttpRequest& req) {
    RouteResult result = {200, "OK", "", "text/plain", ""};
    string bestMatch = req.mtroute.root;
    string reqPath = req.uri;
    bool routeFound = false;


    reqPath.erase(reqPath.begin(), reqPath.begin()+bestMatch.size());
    string fullPath = "/" + req.mtroute.alias + reqPath;

    cout << "the index: " << req.mtroute.index << endl;

    if (isDirectory(fullPath)) {
        if (req.mtroute.index.empty()) {
            // directory listing..
            cout << "need to list directories" << endl;
        }
        else {
            fullPath += req.mtroute.index;
        }
    }

    cout << "full path: " << fullPath << endl;
    
    if (!fileExists(fullPath))
        throw HttpExcept(404, "Not Found");

    ifstream file(fullPath.c_str(), ios::binary);

    if (!file)
        throw HttpExcept(500, "Internal Server Error");
    
    stringstream buffer;
    buffer << file.rdbuf();
    result.responseBody = buffer.str();
    file.close();

    result.contentType = getContentType(fullPath);
    return result;
}
