#pragma once

#include "header.hpp"

struct RouteConfig {
    vector<string> methodes;
    string root;
    string index;
    bool autoindex;
    string redirect;
    string cgi_pass;
    string cgi_extension;
    bool fileUpload;
    string uploadStore;
    RouteConfig() : autoindex(false), fileUpload(false) {}
};

struct ServerConfig {
    string host;
    string port;
    vector<string> server_names;
    map<int, string> error_pages;
    string maxBodySize;
    map<string, RouteConfig> routes;
};

struct WebServerConfig {
    map<string, ServerConfig> servers;
};

class configFile {
private:
    string fileName;
    ifstream cnf;

public:
    configFile(string fileName);
    ~configFile();

    string trim(const string &s);
    size_t parseSize(const string &s);
    void addServer(WebServerConfig &config, ServerConfig &currentServer, RouteConfig &currentRoute, string &currentUri, bool &inServerBlock);
    void ErrorPages(ServerConfig &currentServer);
    void addLocation(ServerConfig &currentServer, RouteConfig &currentRoute, string &currentUri);
    void serverAttributes(ServerConfig &currentServer, const string &line);
    void routesAttributes(RouteConfig &currentRoute, string &currentUri, const string &line);
    WebServerConfig parseConfig();
};

string getKey(ServerConfig currSer);
WebServerConfig configChecking(const string &filePath);