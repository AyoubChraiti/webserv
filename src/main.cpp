#include "../inc/config.hpp"

bool shutServer = false;
int main(int ac, char **av) {
    if (ac == 2) {
        try {
            ctrl_C();
            struct sigaction sa;
            sa.sa_handler = sigchld_handler;
            sigemptyset(&sa.sa_mask);
            sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
            sigaction(SIGCHLD, &sa, NULL);

            mpserv servercnf = configChecking(av[1]);
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
    cout << "here??" << endl;
}
