#pragma once

#include "header.hpp"

struct routeCnf {
    string root; // the rout from the cnf file
    vector<string> methodes;
    string alias;
    string index;
    bool autoindex;
    string redirect;
    bool fileUpload;
    string uploadStore;
    bool cgi;//7
    vector<string> cgi_methods;
    map<string, string> cgi_map;

    routeCnf() : autoindex(false), fileUpload(false), cgi(false) {}
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
    map<string, vector<servcnf> > servers; // Key is host:port
};

class configFile {
private:
    string      fileName;
    ifstream    cnf;
    mpserv      configData;

    size_t parseSize(const string &s);
    string getKey(const servcnf& server);
    vector<string> split(const string &str, char delimiter);
    void parseLine(string &line, servcnf &server, string &section);

public:
    configFile(const string &file);
    const mpserv& parseConfig();
};

mpserv configChecking(const string &filePath);
bool isValidFile(const string &path);
bool isValidDirectory(const string &path);
string trim(const string& str);

