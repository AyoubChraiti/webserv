#include "../include/header.hpp"
#include "../include/config.hpp"

int main(int ac, char **av) {
    if (ac != 2) {
        cout << "Usage: ./webserv <config_file>" << endl;
        return 1;
    }
    /* parsing the config file */
    WebServerConfig serv = configChecking(av[1]);

    /* server setup */
    serverSetup(serv);

}