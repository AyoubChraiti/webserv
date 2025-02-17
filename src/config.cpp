#include "../inc/config.hpp"

configFile::configFile(string fileName) : fileName(fileName) {
    cnf.open(fileName.c_str());
    if (!cnf)
        throw runtime_error("Error: failed to open the config file");
}

configFile::~configFile() {
    if (cnf.is_open())
        cnf.close();
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

void configFile::addServer(mpserv &config, servcnf &currSer, routeCnf &currRout, string &currUri, bool &inServer) {
    addLocation(currSer, currRout, currUri);
    if (inServer) {
        string key = getKey(currSer);
            cout << "KEYS= |" << key << "|" << endl;
        config.servers[key] = currSer;
    }
    currSer = servcnf();
    currRout = routeCnf();
    currUri.clear();
    inServer = true;
}

void configFile::ErrorPages(servcnf &currSer) {
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

void configFile::addLocation(servcnf &currSer, routeCnf &currRout, string &currUri) {
    if (!currUri.empty())
        currSer.routes[currUri] = currRout;
    currRout = routeCnf();
    currUri.clear();
}

void configFile::serverAttributes(servcnf &currSer, const string &line) {
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

void configFile::routesAttributes(routeCnf &currRout, string &currUri, const string &line) {
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

string configFile::getKey(servcnf currSer) {
    string res;
    res = currSer.host;
    res += ":";
    res += currSer.port;
    return res;
}

mpserv configFile::parseConfig() {
    mpserv config;
    string line;
    servcnf currSer;
    bool inServer = false;
    routeCnf currRout;
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
        cout << "KEYS= |" << key << "|" << endl;
        if (config.servers.find(key) == config.servers.end()) {
            config.servers[key] = currSer;
        }
    }
    return config;
}

///////////////////////////////////

void testConfigParser(const string &filePath) {
    try {
        configFile parser(filePath);
        mpserv config = parser.parseConfig();

        cout << "Parsed " << config.servers.size() << " servers.\n";

        map<string, servcnf>::iterator it;
        for (it = config.servers.begin(); it != config.servers.end(); ++it) {
            const string &serverKey = it->first;
            const servcnf &server = it->second;

            cout << "Server (" << serverKey << "):\n";
            cout << "  Host: " << server.host << "\n";
            cout << "  Port: " << server.port << "\n";
            // cout << "  Server Names: ";
            // for (size_t j = 0; j < server.server_names.size(); j++)
            //     cout << server.server_names[j] << " ";
            // cout << "\n  Max Body Size: " << server.maxBodySize << " bytes\n";

            // cout << "  Error Pages:\n";
            // map<int, string>::const_iterator err_it;
            // for (err_it = server.error_pages.begin(); err_it != server.error_pages.end(); ++err_it)
            //     cout << "    " << err_it->first << " -> " << err_it->second << "\n";

            // cout << "  Routes:\n";
            // map<string, routeCnf>::const_iterator route_it;
            // for (route_it = server.routes.begin(); route_it != server.routes.end(); ++route_it) {
            //     const routeCnf &route = route_it->second;
            //     cout << "    URI: " << route_it->first << "\n";
            //     cout << "      Root: " << route.root << "\n";
            //     cout << "      Index: " << route.index << "\n";
            //     cout << "      Autoindex: " << (route.autoindex ? "On" : "Off") << "\n";
            //     cout << "      Redirect: " << route.redirect << "\n";
            //     cout << "      CGI Pass: " << route.cgi_pass << "\n";
            //     cout << "      CGI Extension: " << route.cgi_extension << "\n";
            //     cout << "      File Upload: " << (route.fileUpload ? "Enabled" : "Disabled") << "\n";
            //     cout << "      Upload Store: " << route.uploadStore << "\n";
            //     cout << "      Allowed Methods: ";
            //     for (size_t k = 0; k < route.methodes.size(); k++)
            //         cout << route.methodes[k] << " ";
            //     cout << "\n";
            // }
            cout << "--------------------------------------\n";
        }
    } catch (const exception &e) {
        cerr << "Error: " << e.what() << endl;
    } catch (...) {
        cerr << "Unknown error occurred while parsing the config file." << endl;
    }
}
