#include "../inc/config.hpp"

bool shutServer = false;

void signal_exit(int sig) {
    (void)sig;
    shutServer = true;
    cout << "\nexitingggggggg..........\n" << endl;
}

int main(int ac, char **av) {

    signal(SIGINT, signal_exit);
    signal(SIGTERM, signal_exit);

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
