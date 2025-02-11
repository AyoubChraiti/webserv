#include "../inc/header.hpp"

void sysCallFail() {
    perror("syscall Error");
    exit(1);
}

