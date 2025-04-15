#include "../../inc/config.hpp"

configFile::configFile(const string &file) : fileName(file) {}

size_t configFile::parseSize(const string &s) {
    string str = trim(s);
    size_t pos = 0;
    while (pos < str.size() && isdigit(str[pos]))
        pos++;
    size_t num = atoi((str.substr(0, pos)).c_str());
    if (pos < str.size()) {
        char unit = toupper(str[pos]);
        if (unit == 'K')
            return num * 1024;
        if (unit == 'M')
            return num * 1024 * 1024;
        if (unit == 'G')
            return num * 1024 * 1024 * 1024;
    }
    return num;
}

string configFile::getKey(const servcnf& server) {
    return server.host + ":" + server.port;
}

vector<string> configFile::split(const string &str, char delimiter) {
    vector<string> tokens;
    stringstream ss(str);
    string token;
    while (getline(ss, token, delimiter)) {
        tokens.push_back(trim(token));
    }
    return tokens;
}

string remove_comment(string line) {
    size_t pos = line.find('#');
    if (pos != string::npos) {
        return string(line.substr(0, pos));
    }
    return line;
}

void configFile::parseLine(string &line, servcnf &server, routeCnf &route, string &section) {
    line = remove_comment(line);
    line = trim(line);

    if (line.empty())
        return;

    if (line[0] == '[' && line.back() == ']') {
        section = line.substr(1, line.size() - 2);
        return;
    }

    size_t pos = line.find('=');
    if (pos == string::npos)
        throw runtime_error("Error: syntax issue in the config file.");
    
    string key = trim(line.substr(0, pos));
    string value = trim(line.substr(pos + 1));

    if (key.empty() || value.empty())
        throw runtime_error("Error: syntax issue in the config file.");

    if (section == "server") {
        if (key == "host")
            server.host = getIp(value);
        else if (key == "port")
            server.port = value;
        else if (key == "server_names")
            server.server_names = split(value, ' ');
        else if (key == "client_body_limit")
            server.maxBodySize = value;
    }

    if (section == "error_pages") {
        server.error_pages[atoi(key.c_str())] = value;
    }

    else if (section.rfind("route ", 0) == 0) {
        string routePath = section.substr(6); 
        if (routePath.empty())
            throw runtime_error("Error: syntax issue in the config file.");
        route.root = routePath;
        if (key == "methodes") {
            route.methodes = split(value, ',');
        }
        else if (key == "alias")
            route.alias = value;
        else if (key == "index") {
            route.index = value;
        }
        else if (key == "directory_listing")
            route.autoindex = (value == "on");
        else if (key == "redirect") {
            route.redirect = value;
        }
        else if (key == "cgi_exec") {
            vector<string> cgiData = split(value, ':');
            if (cgiData.size() == 2) {
                route.cgi_extension = cgiData[0];
                route.cgi_pass = cgiData[1];
            }
        }
        else if (key == "allow_upload")
            route.fileUpload = (value == "true");
        else if (key == "upload_directory")
            route.uploadStore = value;

        server.routes[routePath] = route;
    }
}

const mpserv& configFile::parseConfig() {
    cnf.open(fileName);
    if (!cnf.is_open())
        throw runtime_error("Error opening the congif file.");

    string line, section;
    servcnf server;
    routeCnf route;
    bool inserver = false;

    while (getline(cnf, line)) {
        parseLine(line, server, route, section);

        if (line == "[server]") {
            if (inserver) {
                configData.servers[getKey(server)] = server;
                server = servcnf(); 
            }
            inserver = true;
        }
    }

    string key = getKey(server);
    if (inserver && configData.servers.find(key) == configData.servers.end()) {
        configData.servers[key] = server;
    }
    cnf.close();
    return configData;
}

