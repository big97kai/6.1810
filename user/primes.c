#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int newProcess(int p[2]){

    int prime;
    int n;

    close(p[1]);

    if (read(p[0], &prime, 4) != 4) {
        fprintf(2, "Error in read.\n");
        exit(1);
    }
    fprintf(0, "prime %d\n", prime);

    while(read(p[0], &n, 4) == 4){

        if(n % prime != 0){

            break;
        }
    }

    int newP[2];

    if(pipe(newP) == -1){

        fprintf(2, "Error: pipe error.\n");
        exit(1);
    }
    if(fork() == 0) {

        newProcess(newP);
    }else{
        
        close(newP[0]);
        write(newP[1], &n, sizeof(n));
        

        while(read(p[0], &n, 4) == 4){

            if(n % prime != 0){

                write(newP[1], &n, sizeof(n));
            }
        }
        close(p[0]);
        close(newP[1]);
        wait(0);
        exit(0);
    }

    exit(0);
}
int main(int argc, char *argv[])
{
    int p[2];

    if(pipe(p) == -1){

        fprintf(2, "Error: pipe error.\n");
        exit(1);
    }

    if(fork() == 0) {
        newProcess(p);
    } else {
        
        close(p[0]);
        for(int i=2; i<=35; i++){

            if(write(p[1], &i, sizeof(i)) != 4){

                fprintf(2, "Failed to trans %d to his child.\n", &i);
                exit(1);
            }
        }
        close(p[1]);
        wait(0);
        exit(0);
    }
    exit(0);
}