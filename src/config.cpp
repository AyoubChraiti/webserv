#include "../include/config.hpp"

configFile::configFile(string fileName) : fileName(fileName) {
    cnf.open(fileName.c_str());
    if (!cnf)
        throw runtime_error("Error: failed to open the config file");
}

configFile::~configFile() {
    if (cnf.is_open()) {
        cnf.close();
    }
}

string configFile::trim(const string &s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == string::npos)
        return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

size_t configFile::parseSize(const string &s) {
    string str = trim(s);
    size_t pos = 0;
    while (pos < str.size() && isdigit(str[pos]))
        pos++;
    size_t num = atoi((str.substr(0, pos)).c_str());
    if (pos < str.size()) {
        char unit = toupper(str[pos]);
        if (unit == 'K') return num * 1024;
        if (unit == 'M') return num * 1024 * 1024;
        if (unit == 'G') return num * 1024 * 1024 * 1024;
        throw runtime_error("invalid unit: " + string(1, unit));
    }
    return num;
}

void configFile::addServer(WebServerConfig &config, ServerConfig &currSer, RouteConfig &currRout, string &currUri, bool &inServer) {
    addLocation(currSer, currRout, currUri);
    if (inServer) {
        string key = getKey(currSer);
        config.servers[key] = currSer;
    }
    currSer = ServerConfig();
    currRout = RouteConfig();
    currUri.clear();
    inServer = true;
}

void configFile::ErrorPages(ServerConfig &currSer) {
    string line;
    while (getline(cnf, line) && trim(line) != ""
        && line.find("[server.location]") == string::npos) {
        size_t pos = line.find("=");
        if (pos != string::npos) {
            string key = trim(line.substr(0, pos));
            string value = trim(line.substr(pos + 1));

            int code = atoi(key.c_str());
            if (code != 0 || key == "0")
                currSer.error_pages[code] = value;
            else
                cerr << "Invalid error code: " << key << endl;
        }
    }
}

void configFile::addLocation(ServerConfig &currSer, RouteConfig &currRout, string &currUri) {
    if (!currUri.empty())
        currSer.routes[currUri] = currRout;
    currRout = RouteConfig();
    currUri.clear();
}

// todo, convert the port to string and only aftr ur done return it to int..

void configFile::serverAttributes(ServerConfig &currSer, const string &line) {
    size_t pos = line.find("=");
    if (pos != string::npos) {
        string key = trim(line.substr(0, pos));
        string value = trim(line.substr(pos + 1));
        
        if (key == "host")
            currSer.host = value;
        else if (key == "port")
            currSer.port = value;
        else if (key == "server_name") {
            stringstream ss(value);
            string name;
            while (ss >> name)
                currSer.server_names.push_back(name);
        }
        else if (key == "body_size")
            currSer.maxBodySize = value;
    }
}

void configFile::routesAttributes(RouteConfig &currRout, string &currUri, const string &line) {
    size_t pos = line.find("=");
    if (pos != string::npos) {
        string key = trim(line.substr(0, pos));
        string value = trim(line.substr(pos + 1));

        if (key == "uri")
            currUri = value;
        else if (key == "root")
            currRout.root = value;
        else if (key == "methods") {
            stringstream ss(value);
            string method;
            while (ss >> method)
                currRout.methodes.push_back(method);
        }
        else if (key == "index")
            currRout.index = value;
        else if (key == "upload") {
            currRout.fileUpload = true;
            currRout.uploadStore = value;
        }
        else if (key == "autoindex")
            currRout.autoindex = (value == "on");
    }
}

string getKey(ServerConfig currSer) {
    string res;
    res = currSer.host;
    res += ":";
    res += currSer.port;
    return res;
}

WebServerConfig configFile::parseConfig() {
    WebServerConfig config;
    string line;
    ServerConfig currSer;
    bool inServer = false;
    RouteConfig currRout;
    string currUri;

    while (getline(cnf, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#')
            continue;

        if (line == "[server]")
            addServer(config, currSer, currRout, currUri, inServer);
        else if (line == "[server.errors]")
            ErrorPages(currSer);
        else if (line == "[server.location]")
            addLocation(currSer, currRout, currUri);
        else {
            serverAttributes(currSer, line);
            routesAttributes(currRout, currUri, line);
        }
    }
    addLocation(currSer, currRout, currUri);
    if (inServer) {
        string key = getKey(currSer);
        if (config.servers.find(key) == config.servers.end()) {
            config.servers[key] = currSer;
        }
    }
    return config;
}

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

WebServerConfig configChecking(const string &filePath) {
    try {
        configFile parser(filePath);
        WebServerConfig config = parser.parseConfig();

        if (config.servers.empty())
            throw runtime_error("Error: No servers found in the configuration.");

        map<string, ServerConfig>::iterator it;
        for (it = config.servers.begin(); it != config.servers.end(); ++it) {
            const ServerConfig &server = it->second;

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

            map<string, RouteConfig>::const_iterator route_it;
            for (route_it = server.routes.begin(); route_it != server.routes.end(); ++route_it) {
                const RouteConfig &route = route_it->second;

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




///////////////////////////////////

// void testConfigParser(const string &filePath) {
//     try {
//         configFile parser(filePath);
//         WebServerConfig config = parser.parseConfig();

//         cout << "Parsed " << config.servers.size() << " servers.\n";

//         map<string, ServerConfig>::iterator it;
//         for (it = config.servers.begin(); it != config.servers.end(); ++it) {
//             const string &serverKey = it->first;
//             const ServerConfig &server = it->second;

//             cout << "Server (" << serverKey << "):\n";
//             cout << "  Host: " << server.host << "\n";
//             cout << "  Port: " << server.port << "\n";
//             cout << "  Server Names: ";
//             for (size_t j = 0; j < server.server_names.size(); j++)
//                 cout << server.server_names[j] << " ";
//             cout << "\n  Max Body Size: " << server.maxBodySize << " bytes\n";

//             cout << "  Error Pages:\n";
//             map<int, string>::const_iterator err_it;
//             for (err_it = server.error_pages.begin(); err_it != server.error_pages.end(); ++err_it)
//                 cout << "    " << err_it->first << " -> " << err_it->second << "\n";

//             cout << "  Routes:\n";
//             map<string, RouteConfig>::const_iterator route_it;
//             for (route_it = server.routes.begin(); route_it != server.routes.end(); ++route_it) {
//                 const RouteConfig &route = route_it->second;
//                 cout << "    URI: " << route_it->first << "\n";
//                 cout << "      Root: " << route.root << "\n";
//                 cout << "      Index: " << route.index << "\n";
//                 cout << "      Autoindex: " << (route.autoindex ? "On" : "Off") << "\n";
//                 cout << "      Redirect: " << route.redirect << "\n";
//                 cout << "      CGI Pass: " << route.cgi_pass << "\n";
//                 cout << "      CGI Extension: " << route.cgi_extension << "\n";
//                 cout << "      File Upload: " << (route.fileUpload ? "Enabled" : "Disabled") << "\n";
//                 cout << "      Upload Store: " << route.uploadStore << "\n";
//                 cout << "      Allowed Methods: ";
//                 for (size_t k = 0; k < route.methodes.size(); k++)
//                     cout << route.methodes[k] << " ";
//                 cout << "\n";
//             }
//             cout << "--------------------------------------\n";
//         }
//     } catch (const exception &e) {
//         cerr << "Error: " << e.what() << endl;
//     } catch (...) {
//         cerr << "Unknown error occurred while parsing the config file." << endl;
//     }
// }
