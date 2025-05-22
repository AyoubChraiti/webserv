#include "../../inc/config.hpp"

bool isValidPort(const string &portStr) {
    if (portStr.empty())
        return false;
    for (size_t i = 0; i < portStr.size(); ++i) {
        if (!isdigit(portStr[i]))
            return false;
    }
    int port = atoi(portStr.c_str());
    return (port >= 1024 && port <= 65535);
}

bool isValidHost(const string &host) {
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;

    if (getaddrinfo(host.c_str(), NULL, &hints, &res) != 0) {
        return false;
    }
    freeaddrinfo(res);
    return true;
}

bool isValidBodySize(const string& value) {
    if (value.empty())
        return false;

    char suffix = value[value.size() - 1];
    bool hasSuffix = (suffix == 'K'|| suffix == 'M'|| suffix == 'G');
    
    string numericValue = hasSuffix ? value.substr(0, value.size() - 1) : value;

    for (size_t i = 0; i < numericValue.size(); ++i) {
        if (!isdigit(numericValue[i]))
            return false;
    }
    if (numericValue.empty())
        return false;
    return true;
}

bool isValidMethod(const string &method) {
    return method == "GET" || method == "POST" || method == "DELETE";
}

mpserv configChecking(const string &filePath) {
    configFile parser(filePath);
    mpserv config = parser.parseConfig();

    if (config.servers.empty())
        throw runtime_error("Error: No servers found in the configuration.");

    map<string, servcnf>::iterator it;
    for (it = config.servers.begin(); it != config.servers.end(); ++it) {
        const servcnf &server = it->second;

        if (!isValidHost(server.host))
            throw runtime_error("Error: a Server has an invalid host");

        if (!isValidPort(server.port))
            throw runtime_error("Error: a Server has an invalid port");

        if (!isValidBodySize(server.maxBodySize))
            throw runtime_error("Error: body size syntax issue");

        map<int, string>::const_iterator err_it;
        for (err_it = server.error_pages.begin(); err_it != server.error_pages.end(); ++err_it) {
            if (err_it->first < 100 || err_it->first > 599)
                throw runtime_error("Error: invalid error code");
            if (!isValidFile(err_it->second))
                throw runtime_error("Error: Error page file does not exist: " + err_it->second);
        }

        for (map<string, routeCnf>::const_iterator route_it = server.routes.begin(); route_it != server.routes.end(); ++route_it) {
            const routeCnf &route = route_it->second;

            if (route_it->first.empty() || route_it->first[0] != '/')
                throw runtime_error("Error: Route '" + route_it->first + "' has an invalid or missing URI.");

            if (route.alias.empty())
                throw runtime_error("Error: Route '" + route_it->first + "' is missing alias directory.");
            if (!isValidDirectory(route.alias))
                throw runtime_error("Error: alias directory '" + route.alias + "' does not exist.");

            vector<string>::const_iterator method_it;
            for (method_it = route.methodes.begin(); method_it != route.methodes.end(); ++method_it) {
                if (!isValidMethod(*method_it))
                    throw runtime_error("Error: Route '" + route_it->first + "' has an invalid HTTP method '" + *method_it + "'.");
            }

            if (!route.uploadStore.empty() && !isValidDirectory(route.uploadStore))
                throw runtime_error("Error: Upload directory '" + route.uploadStore + "' does not exist.");
                
            if (route.cgi) {
                if (route.cgi_map.empty())
                    throw runtime_error("Error: Route '" + route_it->first + "' has CGI enabled but no CGI extensions defined.");
                if (route.cgi_methods.empty())
                    throw runtime_error("Error: Route '" + route_it->first + "' has CGI enabled but no CGI methods defined.");
            }
            else {
                if (!route.cgi_map.empty())
                    throw runtime_error("Error: Route '" + route_it->first + "' has CGI disabled but CGI extensions are defined.");
                if (!route.cgi_methods.empty())
                    throw runtime_error("Error: Route '" + route_it->first + "' has CGI disabled but CGI methods are defined.");
            }
        }
    }
    return config;
}
