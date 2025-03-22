#pragma once

#include "config.hpp"
#include "header.hpp"
#include "request.hpp"

/* responce class */





void handle_client_write(int clientFd, map<int, HttpRequest>& requestStates);