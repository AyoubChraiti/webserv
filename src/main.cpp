#include "../inc/config.hpp"

bool shutServer = false;

/* function to get used to config struct*/

// void PrintConfigFile(mpserv& conf) 
// {

//     for (auto it = conf.servers.begin() ; it != conf.servers.end(); it++)
//     {
//         cout << "[server]" << endl;
//         cout << "host = " << it->second.host << endl;
//         cout << "port = " <<  it->second.port << endl;
//         cout << "body_size = " << it->second.maxBodySize << endl;
//         for (auto second = it->second.server_names.begin(); second != it->second.server_names.end(); second++)
//         {
//             if (second == it->second.server_names.begin())
//                 cout << "server_name = ";
//             if (second == it->second.server_names.end() - 1)
//                 cout << *second << endl;
//             else
//                 cout << *second << " ";
//         }
//         cout << endl << "[server.errors]" << endl;
//         cout << it->second.error_pages.begin()->first << " = "
//          << it->second.error_pages.begin()->second << endl;

//         for (auto routes = it->second.routes.begin(); routes != it->second.routes.end(); routes++)
//         {
//             cout << endl <<"[server.location]" << endl;
//             cout << "uri = " << routes->first << endl;
//             cout << "root = " << routes->second.root << endl;
//             cout << "methods = ";
//             for (auto methods = routes->second.methodes.begin(); methods != routes->second.methodes.end() ; methods++)
//                 cout << *methods << " " ;
//             cout << endl << "index = " << routes->second.index << endl;
//             cout << "upload = " << routes->second.uploadStore << endl;
//             cout << "autoindex = " << routes->second.autoindex << endl;
//         }
//     }
// }




void signal_exit(int sig) {
    (void)sig;
    shutServer = true;
    cout << "\nexitingggggggg.........." << endl;
}

int main(int ac, char **av) {

    signal(SIGINT, signal_exit);
    signal(SIGTERM, signal_exit);

    if (ac == 2) {
        try {
            mpserv serverCongif = configChecking(av[1]);
            // PrintConfigFile(serverCongif);
            // return 1;
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
