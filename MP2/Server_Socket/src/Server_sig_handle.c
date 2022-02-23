#include <stdio.h>
#include <unistd.h>
#include "Server_sig_handle.h"

// Function to handle child processes
void child_proc(int signal){
    pid_t pid;
    // Non-blocking
    pid = waitpid(-1, NULL, WNOHANG);
    printf("[Remove child process] ID: %d\n", pid);
}

// Function to control the action while detecting SIGCHLD
int handle_child_proc(struct sigaction* action){
    action->sa_handler = child_proc;
    action->sa_flags = 0;
    sigemptyset(&action->sa_mask);

    return sigaction(SIGCHLD, action, 0);
}
