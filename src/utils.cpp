#include "../include/header.hpp"

void sysCallFail() {
    perror("syscall Error");
    exit(1);
}

