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
    vector<string> server_names;
    map<int, string> error_pages;
    string maxBodySize;
    map<string, routeCnf> routes; // uri is the key

    servcnf() : maxBodySize("1048576") {} // 1mb par defaut
};

struct mpserv {
    map<string, servcnf> servers; // Key is host:port
};

class configFile {
private:
    string      fileName;
    ifstream    cnf;
    mpserv      configData;

    size_t parseSize(const string &s);
    string getKey(const servcnf& server);
    vector<string> split(const string &str, char delimiter);
    string removeComment(const string &str);
    void parseLine(string &line, servcnf &server, routeCnf &route, string &section);

public:
    configFile(const string &file);
    bool parseConfig();
    const mpserv &getConfigData() const;
};

mpserv configChecking(const string &filePath);
bool isValidFile(const string &path);
bool isValidDirectory(const string &path);
string trim(const string& str);