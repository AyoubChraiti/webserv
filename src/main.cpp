#include "../inc/config.hpp"

bool shutServer = false;


void printConfig(const mpserv &config) {
    for (map<string, servcnf>::const_iterator it = config.servers.begin(); it != config.servers.end(); ++it) {
        cout << "[Server] " << it->first << endl;
        const servcnf &server = it->second;
        cout << "  Host: " << server.host << "\n";
        cout << "  Port: " << server.port << "\n";
        cout << "  Server Names: ";
        for (size_t i = 0; i < server.server_names.size(); ++i)
            cout << server.server_names[i] << " ";
        cout << "\n  Max Body Size: " << server.maxBodySize << endl;

        cout << "  Error Pages:\n";
        for (map<int, string>::const_iterator ep = server.error_pages.begin(); ep != server.error_pages.end(); ++ep)
            cout << "    " << ep->first << ": " << ep->second << endl;

        cout << "  Routes:\n";
        for (map<string, routeCnf>::const_iterator rt = server.routes.begin(); rt != server.routes.end(); ++rt) {
            cout << "    [" << rt->first << "]\n";
            const routeCnf &route = rt->second;
            cout << "      Root: " << route.root << "\n";
            cout << "      Alias: " << route.alias << "\n";
            cout << "      Index: " << route.index << "\n";
            cout << "      AutoIndex: " << (route.autoindex ? "on" : "off") << "\n";
            cout << "      Redirect: " << route.redirect << "\n";
            cout << "      File Upload: " << (route.fileUpload ? "true" : "false") << "\n";
            cout << "      Upload Store: " << route.uploadStore << "\n";
            cout << "      CGI: " << (route.cgi ? "on" : "off") << "\n";

            if (!route.methodes.empty()) {
                cout << "      Methods: ";
                for (size_t i = 0; i < route.methodes.size(); ++i)
                    cout << route.methodes[i] << " ";
                cout << "\n";
            }

            if (!route.cgi_methods.empty()) {
                cout << "      CGI Methods: ";
                for (size_t i = 0; i < route.cgi_methods.size(); ++i)
                    cout << route.cgi_methods[i] << " ";
                cout << "\n";
            }

            if (!route.cgi_map.empty()) {
                cout << "      CGI Map:\n";
                for (map<string, string>::const_iterator cm = route.cgi_map.begin(); cm != route.cgi_map.end(); ++cm)
                    cout << "        " << cm->first << " => " << cm->second << "\n";
            }
        }
        cout << "\n";
    }
}


int main(int ac, char **av) {
    if (ac == 2) {
        try {
            ctrl_C();
            mpserv servercnf = configChecking(av[1]);
            printConfig(servercnf);
            webserver(servercnf);
        }
        catch (const exception &e) {
            cerr << e.what() << endl;
        }
    }
    else {
        cout << "Usage: ./webserv <config_file>" << endl;
        return 1;
    }
}
