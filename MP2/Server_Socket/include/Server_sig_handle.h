#ifndef SERVER_SIG_HANDLE
#define SERVER_SIG_HANDLE

#include <sys/wait.h>
#include <signal.h>

void child_proc(int);
int handle_child_proc(struct sigaction*);

#endif
