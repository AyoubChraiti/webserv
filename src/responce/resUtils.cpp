#include "../../inc/responce.hpp"
#include "../../inc/request.hpp"

bool isDirectory(const string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        return (st.st_mode & S_IFDIR) != 0;
    }
    return false;
}

bool fileExists(const string& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0 && (st.st_mode & S_IFREG);
}

string getContentType(const string& filepath) {
    map<string, string> mimeTypes;

    mimeTypes[".7z"]    = "application/x-7z-compressed";
    mimeTypes[".avi"]   = "video/x-msvideo";
    mimeTypes[".bat"]   = "application/x-msdownload";
    mimeTypes[".bin"]   = "application/octet-stream";
    mimeTypes[".bmp"]   = "image/bmp";
    mimeTypes[".css"]   = "text/css";
    mimeTypes[".csv"]   = "text/csv";
    mimeTypes[".doc"]   = "application/msword";
    mimeTypes[".docx"]  = "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
    mimeTypes[".dll"]   = "application/octet-stream";
    mimeTypes[".exe"]   = "application/octet-stream";
    mimeTypes[".eot"]   = "application/vnd.ms-fontobject";
    mimeTypes[".gif"]   = "image/gif";
    mimeTypes[".gz"]    = "application/gzip";
    mimeTypes[".html"]  = "text/html";
    mimeTypes[".ico"]   = "image/x-icon";
    mimeTypes[".iso"]   = "application/octet-stream";
    mimeTypes[".js"]    = "text/javascript";
    mimeTypes[".jpg"]   = "image/jpeg";
    mimeTypes[".jpeg"]  = "image/jpeg";
    mimeTypes[".json"]  = "application/json";
    mimeTypes[".java"]  = "text/x-java-source";
    mimeTypes[".mjs"]   = "text/javascript";
    mimeTypes[".mp3"]   = "audio/mpeg";
    mimeTypes[".mp4"]   = "video/mp4";
    mimeTypes[".mov"]   = "video/quicktime";
    mimeTypes[".mkv"]   = "video/x-matroska";
    mimeTypes[".ogg"]   = "audio/ogg";
    mimeTypes[".odt"]   = "application/vnd.oasis.opendocument.text";
    mimeTypes[".ods"]   = "application/vnd.oasis.opendocument.spreadsheet";
    mimeTypes[".odp"]   = "application/vnd.oasis.opendocument.presentation";
    mimeTypes[".otf"]   = "font/otf";
    mimeTypes[".png"]   = "image/png";
    mimeTypes[".pdf"]   = "application/pdf";
    mimeTypes[".ppt"]   = "application/vnd.ms-powerpoint";
    mimeTypes[".pptx"]  = "application/vnd.openxmlformats-officedocument.presentationml.presentation";
    mimeTypes[".php"]   = "application/x-httpd-php";
    mimeTypes[".py"]    = "text/x-python";
    mimeTypes[".rar"]   = "application/x-rar-compressed";
    mimeTypes[".rtf"]   = "application/rtf";
    mimeTypes[".svg"]   = "image/svg+xml";
    mimeTypes[".sh"]    = "application/x-sh";
    mimeTypes[".sfnt"]  = "font/sfnt";
    mimeTypes[".txt"]   = "text/plain";
    mimeTypes[".tiff"]  = "image/tiff";
    mimeTypes[".tar"]   = "application/x-tar";
    mimeTypes[".ttf"]   = "font/ttf";
    mimeTypes[".webp"]  = "image/webp";
    mimeTypes[".wav"]   = "audio/wav";
    mimeTypes[".webm"]  = "video/webm";
    mimeTypes[".woff"]  = "font/woff";
    mimeTypes[".woff2"] = "font/woff2";
    mimeTypes[".xml"]   = "application/xml";
    mimeTypes[".xls"]   = "application/vnd.ms-excel";
    mimeTypes[".xlsx"]  = "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
    mimeTypes[".zip"]   = "application/zip";

    size_t extPos = filepath.rfind('.');
    if (extPos == string::npos) {
        return "application/octet-stream";
    }

    string fileExt = filepath.substr(extPos);
    for (size_t i = 0; i < fileExt.length(); ++i) {
        fileExt[i] = static_cast<char>(::tolower(static_cast<unsigned char>(fileExt[i])));
    }

    map<string, string>::const_iterator it = mimeTypes.find(fileExt);
    if (it != mimeTypes.end()) {
        return it->second;
    }
    return "application/octet-stream";
}

void closeOrSwitch(int clientFd, int epollFd, HttpRequest& req, map<int, HttpRequest>& requestmp) {
    if (req.connection != "keep-alive") {
        requestmp.erase(clientFd);
        if (epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, NULL) == -1) {
            perror("epoll_ctl");
        }
        close(clientFd);
    }
    else {
        cerr << "cnx want to keep the cnx" << endl;
        requestmp.erase(clientFd);
        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = clientFd;
        if (epoll_ctl(epollFd, EPOLL_CTL_MOD, clientFd, &ev) == -1) {
            perror("epoll_ctl");
        }
    }
}

string generateAutoIndex(const string& fullPath, const string& uriPath) {
    stringstream html;

    html << "<html><head><title>Index of " << uriPath << "</title></head><body>";
    html << "<h1>Index of " << uriPath << "</h1><hr><pre>";

    DIR* dir = opendir(fullPath.c_str());
    if (!dir) {
        return "<h1>403 Forbidden</h1>";
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        string name = entry->d_name;

        if (name == ".")
            continue;

        string displayName = name;
        string href = uriPath;
        if (href.back() != '/') href += "/";
        href += name;

        string fullEntryPath = fullPath + "/" + name;

        struct stat st;
        if (stat(fullEntryPath.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
            displayName += "/";
            href += "/";
        }

        html << "<a href=\"" << href << "\">" << displayName << "</a>\n";
    }
    closedir(dir);
    html << "</pre><hr></body></html>";
    return html.str();
}
