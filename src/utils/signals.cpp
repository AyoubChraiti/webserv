#include "../../inc/header.hpp"

void signal_exit(int sig) {
    (void)sig;
    shutServer = true;
    cout << "\n Ctrl-C trigered: exiting.." << endl;
    // exit(1);
}

void ctrl_C() {
    signal(SIGINT, signal_exit);
    signal(SIGTERM, signal_exit);
}
