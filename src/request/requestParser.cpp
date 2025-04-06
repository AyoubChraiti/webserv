#include "../../inc/request.hpp"

void checkMethod(const string& method) {
    if (method != "GET" && method != "POST" && method != "DELETE")
        throw HttpExcept(405, "Unsupported method: " + method);
}

void checkURI(const string& path) {
    if (path.empty() || path[0] != '/')
        throw HttpExcept(400, "Invalid path: " + path);

    const string badChars = " \"<>\\^`{}|";
    for (size_t i = 0; i < path.length(); ++i) {
        if (!isprint(path[i]) || badChars.find(path[i]) != string::npos)
            throw HttpExcept(400, "Bad character in URI: " + path);
    }
    if (path.length() > 2048)
        throw HttpExcept(414, "URI too long: " + to_string(path.length()));
}

void checkHeaders(const HttpRequest& req) {
    if (req.headers.count("Transfer-Encoding")) {
        string enc = req.headers.at("Transfer-Encoding");
        transform(enc.begin(), enc.end(), enc.begin(), ::tolower);
        if (enc != "chunked")
            throw HttpExcept(501, "Unsupported Transfer-Encoding: " + enc);
    }
    if (req.method == "POST" &&
        !req.headers.count("Content-Length") &&
        !req.headers.count("Transfer-Encoding")) {
        throw HttpExcept(400, "Missing Content-Length or Transfer-Encoding");
    }
}

void getRoute(const servcnf& server, const string& path, string& matched, HttpRequest& req) { // matches to the conf routes
    const routeCnf* route = NULL;
    matched.clear();

    for (map<string, routeCnf>::const_iterator it = server.routes.begin(); it != server.routes.end(); ++it) {
        if (path.find(it->first) == 0 && it->first.length() > matched.length()) {
            matched = it->first;
            route = &it->second;
        }
    }
    if (!route)
        throw HttpExcept(404, "No route for path: " + path);
    req.mtroute = *route;
}

void checkAllowed(routeCnf route, const string& method, const string& path) {
    if (find(route.methodes.begin(), route.methodes.end(), method) == route.methodes.end()) {
        throw HttpExcept(405, "Method not allowed: " + method + " for route " + path);
    }
}

void checkBody(const servcnf& server, const HttpRequest& req) {
    if (req.method == "POST" && !server.maxBodySize.empty()) {
        size_t maxSize = strtoul(server.maxBodySize.c_str(), NULL, 10);
        if (req.body.size() > maxSize)
            throw HttpExcept(413, "Body too large: " + to_string(req.body.size()));
    }
}
