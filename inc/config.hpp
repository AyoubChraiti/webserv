#pragma once

#include "header.hpp"

struct routeCnf {
    vector<string> methodes;
    string root;
    string index;
    bool autoindex;
    string redirect;
    string cgi_pass;
    string cgi_extension;
    bool fileUpload;
    string uploadStore;
    routeCnf() : autoindex(false), fileUpload(false) {}
};

struct servcnf {
    string host;
    string port;
    string maxBodySize;
    vector<string> server_names;
    map<int, string> error_pages;
    map<string, routeCnf> routes; // the uri is the key here
};

struct mpserv {
    map<string, servcnf> servers; // the key here is host:port
};

class configFile {
private:
    string fileName;
    ifstream cnf;

public:
    configFile(string fileName);
    ~configFile();

    size_t parseSize(const string &s);
    void addServer(mpserv &config, servcnf &currentServer, routeCnf &currentRoute, string &currentUri, bool &inServerBlock);
    void ErrorPages(servcnf &currentServer);
    void addLocation(servcnf &currentServer, routeCnf &currentRoute, string &currentUri);
    void serverAttributes(servcnf &currentServer, const string &line);
    void routesAttributes(routeCnf &currentRoute, string &currentUri, const string &line);
    mpserv parseConfig();
    string getKey(servcnf currSer);
};

mpserv configChecking(const string &filePath);
