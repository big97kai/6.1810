#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    int p[2];

    if(pipe(p) == -1){

        fprintf(2, "Error: pipe error.\n");
    }

    if(fork() == 0) {
        char buffer[1];
        read(p[0], buffer, 1);
        close(p[0]);
        fprintf(0, "%d: received ping\n", getpid());
        write(p[1], buffer, 1);
        close(p[1]);
    } else {
        char buffer[1];
        buffer[0] = 'a';
        write(p[1], buffer, 1);
        close(p[1]);
        read(p[0], buffer, 1);
        close(p[0]);
        fprintf(0, "%d: received pong\n", getpid());
    }

    exit(0);
}