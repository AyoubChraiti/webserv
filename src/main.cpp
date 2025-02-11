#include "../inc/config.hpp"

int main(int ac, char **av) {
    if (ac != 2) {
        cout << "Usage: ./webserv <config_file>" << endl;
        return 1;
    }
    /*parsing conf file*/
    mpserv serverCongif = configChecking(av[1]);

    /*server part*/
    webserver(serverCongif);
}

