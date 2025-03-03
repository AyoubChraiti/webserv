#include "../inc/config.hpp"

int main(int ac, char **av) {
    if (ac == 2) {
        try {
            mpserv serverCongif = configChecking(av[1]);
            webserver(serverCongif);
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
