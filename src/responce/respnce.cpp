// #include "../../inc/responce.hpp"

// bool isDirectory(const string& path) {
//     struct stat st;
//     if (stat(path.c_str(), &st) == 0) {
//         return (st.st_mode & S_IFDIR) != 0;
//     }
//     return false;
// }

// bool fileExists(const string& path) {
//     ifstream file(path.c_str());
//     bool exists = file.good();
//     file.close();
//     return exists;
// }

// void handle_client_write(int clientFd, int epollFd, mpserv& conf, map<int, HttpRequest>& requestStates) {
//     map<int, HttpRequest>::iterator it = requestStates.find(clientFd);
//     if (it == requestStates.end()) { // wont even need this ig
//         Response res;
//         res.setStatus(500);
//         res.setBody("Internal Error: No request state");
//         res.send(clientFd);
//         close(clientFd);
//         epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, NULL);
//         return;
//     }

//     HttpRequest& req = it->second;
//     servcnf& server = req.conf;
//     Response res;

//     if (req.method != "GET") {
//         res.setStatus(501);
//         res.setBody("Method Not Implemented: " + req.method);
//     }
//     else {
//         string matchedUri;
//         routeCnf* route = 0;
//         string::size_type longestMatch = 0;

//         map<string, routeCnf>::iterator routeIt;
//         for (routeIt = server.routes.begin(); routeIt != server.routes.end(); ++routeIt) {
//             const string& uri = routeIt->first;
//             if (req.path.find(uri) == 0 && uri.length() > longestMatch) {
//                 vector<string>& methods = routeIt->second.methodes;
//                 bool methodAllowed = false;
//                 for (vector<string>::iterator mIt = methods.begin(); mIt != methods.end(); ++mIt) {
//                     if (*mIt == req.method) {
//                         methodAllowed = true;
//                         break;
//                     }
//                 }
//                 if (methodAllowed) {
//                     matchedUri = uri;
//                     route = &routeIt->second;
//                     longestMatch = uri.length();
//                 }
//             }
//         }

//         if (!route) {
//             res.setStatus(405);
//             res.setBody("No route found for " + req.method + " " + req.path);
//         }
//         else {
//             if (!route->redirect.empty()) {
//                 res.setRedirect(route->redirect);
//             }
//             else {
//                 string remainingPath = (matchedUri == "/" && req.path == "/") ? "" : req.path.substr(matchedUri.length());
//                 string filePath = route->root + remainingPath;

//                 // cout << "file Path= " << filePath << endl;

//                 if (fileExists(filePath)) {
//                     if (isDirectory(filePath) && !route->index.empty()) {
//                         string indexPath = filePath + (filePath[filePath.length()-1] == '/' ? "" : "/") + route->index;
//                         if (fileExists(indexPath) && !isDirectory(indexPath)) {
//                             filePath = indexPath;
//                         }
//                         else {
//                             res.setStatus(403);
//                             res.setBody("Directory listing not allowed or no index file");
//                         }
//                     }

//                     if (res.getStatusCode() == 200 && !isDirectory(filePath)) {
//                         if (!res.setFile(filePath)) {
//                             res.setStatus(500);
//                             res.setBody("Failed to open file");
//                         }
//                     }
//                 }
//                 else {
//                     res.setStatus(404);
//                     res.setBody("Resource not found: " + req.path);
//                 }
//             }
//         }
//     }
//     res.send(clientFd);

//     close(clientFd);
//     epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, NULL);
//     requestStates.erase(clientFd);
// }

