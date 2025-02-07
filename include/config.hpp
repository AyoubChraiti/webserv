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
    ifstream cnf;

public:
    configFile(string file_name) : file_name(file_name) {
        cnf.open(file_name);
        if (!cnf)
            throw "Error: failed to open the config file";
    }
    ~configFile() {
        if (cnf.is_open()) {
            cnf.close();
        }
    }

};

void config_file(string file);
