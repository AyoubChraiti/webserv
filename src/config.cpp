#include "../include/config.hpp"

void config_file(string file) {
    try {
        configFile conf(file);
    }
    catch(const char *err) {
        cout << err << endl;
        return;
    }
}
