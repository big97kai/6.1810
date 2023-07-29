#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

int xargs(char* exec_name, char** parameter, int parameter_size){


    char pipe_in[MAXARG];

    memset(pipe_in, 0, sizeof(pipe_in));
    int i=0;
    char temp;
    while(read(0, &temp, sizeof(char))){
        if(temp != '\n'){

            pipe_in[i++] = temp;
        }else{

            pipe_in[i] = '\0';
            if(fork() > 0){

                wait(0);
            }else{
                char* child_parameter[parameter_size + 3];
                child_parameter[0] = exec_name;
                for(int i=1; i<=parameter_size; i++){

                    child_parameter[i] = parameter[i];
                }
                child_parameter[parameter_size+1] = pipe_in;
                child_parameter[parameter_size+2] = 0;
                exec(exec_name, child_parameter);
            }
            i=0;
        }
    }

    exit(0);
}
 
int main(int argc, char *argv[])
{
    char *exec_name = "echo";
    if(argc > 2){

        exec_name = argv[1];
        argc--;
    }
    xargs(exec_name, argv+1, argc-1);
    exit(0);
}