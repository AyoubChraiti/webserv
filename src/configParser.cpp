#include "../inc/config.hpp"

bool isValidPort(const string &portStr) {
    if (portStr.empty())
        return false;
    for (size_t i = 0; i < portStr.size(); ++i) {
        if (!isdigit(portStr[i]))
            return false;
    }
    int port = atoi(portStr.c_str());
    return (port >= 1 && port <= 65535);
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

mpserv configChecking(const string &filePath) {
    try {
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

            if (server.server_names.empty())
                throw runtime_error("Error: the server name is not provided");

            if (!isValidBodySize(server.maxBodySize))
                throw runtime_error("Error: body size syntax issue");

            map<int, string>::const_iterator err_it;
            for (err_it = server.error_pages.begin(); err_it != server.error_pages.end(); ++err_it) {
                if (err_it->second.empty())
                    throw runtime_error("Error: Error page for code " + to_string<int>(err_it->first) + " in server is empty.");
            }

            map<string, routeCnf>::const_iterator route_it;
            for (route_it = server.routes.begin(); route_it != server.routes.end(); ++route_it) {
                const routeCnf &route = route_it->second;

                if (route.root.empty())
                    throw runtime_error("Error: Route '" + route_it->first + "' in server has no root directory.");

                if (!route.methodes.empty()) {
                    vector<string>::const_iterator method_it;
                    for (method_it = route.methodes.begin(); method_it != route.methodes.end(); ++method_it) {
                        if (*method_it != "GET" && *method_it != "POST" && *method_it != "DELETE")
                            throw runtime_error("Error: Route '" + route_it->first + "' a server has invalid HTTP method '" + *method_it + "'.");
                    }
                }
            }
        }
        return config;
    }
    catch (const exception &e) {
        cerr << "Config file " << e.what() << endl;
        exit(EXIT_FAILURE);
    }
}
