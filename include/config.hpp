#pragma once

#include "header.hpp"

#include <string>
#include <map>
#include <vector>

struct RouteConfig {
    vector<string> methodes;
    string root;
    string index;
    bool autoindex;
    string redirect;
    string cgi_pass;
    string cgi_extension;
    bool file_upload;
    string upload_store;
    RouteConfig() : autoindex(false), file_upload(false) {}
};

struct ServerConfig {
    string host;
    int port;
    vector<string> server_names;
    map<int, string> error_pages;
    size_t client_max_body_size;
    map<string, RouteConfig> routes;
    ServerConfig() : host("0.0.0.0"), port(80), client_max_body_size(1024 * 1024) {}
};

struct WebServerConfig {
    vector<ServerConfig> servers;
};

class configFile {
private:
    string file_name;
public:
    configFile(string file_name) : file_name(file_name) {
        
    }

};
