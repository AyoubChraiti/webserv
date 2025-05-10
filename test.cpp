#include <map>
#include <string>
#include <stdexcept>
#include <iostream>

struct HttpExcept : public std::runtime_error {
    int status;
    HttpExcept(int s, const std::string& msg)
        : std::runtime_error(msg), status(s) {}
};

struct RouteCnf {
    // your route configuration fields
};

// Improved route-matching function with segment-boundary check
// Based on RFC 3986 §3.3, we treat "/" as the segment delimiter and ensure matches occur only at segment boundaries.
std::string matchRoute(const std::string& uri, const std::map<std::string, RouteCnf>& routes) {
    size_t bestLength = 0;
    std::string bestKey;
    bool found = false;

    for (const auto& entry : routes) {
        const std::string& prefix = entry.first;
        // Check for exact prefix match
        if (uri.size() >= prefix.size() && uri.compare(0, prefix.size(), prefix) == 0) {
            // Enforce segment boundary: either prefix ends with '/' or next char is '/'
            std::cout << prefix << std::endl;
            if (prefix.back() != '/' && uri.size() > prefix.size() && uri[prefix.size()] != '/') {
                continue;
            }
            // If multiple prefixes match, choose the longest one
            if (prefix.size() > bestLength) {
                bestLength = prefix.size();
                bestKey = prefix;
                found = true;
            }
        }
    }

    if (!found) {
        throw HttpExcept(404, "No route for path: " + uri);
    }

    return bestKey;
}

// Example usage
typedef std::map<std::string, RouteCnf> RouteMap;

int main() {
    RouteMap routes = {
        {"/jo",    {/*...*/}},
        {"/job/",  {/*...*/}},
        {"/home",   {/*...*/}}
    };

    std::string uri = "/home/job/index.html";
    try {
        std::string routeKey = matchRoute(uri, routes);
        std::cout << routeKey << std::endl;  // prints "/home/job/" if configured, else throws
    } catch (const HttpExcept& ex) {
        std::cerr << "Error " << ex.status << ": " << ex.what() << std::endl;
    }

    return 0;
}
