/****************
LE2: Basic Shell
****************/
#include <unistd.h> // pipe, fork, dup2, execvp, close
#include <sys/wait.h> // wait
#include "Tokenizer.h"
#include <iostream>
using namespace std;

int main () {
    // TODO: insert lab exercise 2 main code here
    pid_t pid;
    int fd[2];
    // // lists all the files in the root directory in the long format
    // char* cmd1[] = {(char*) "ls", (char*) "-al", (char*) "/", nullptr};
    // // translates all input from lowercase to uppercase
    // char* cmd2[] = {(char*) "tr", (char*) "a-z", (char*) "A-Z", nullptr};

    vector <char*> arg; 
    // int status;

    //save original input and output
    int stdout = dup(1);
    int stdin = dup(0);
    string input;

    while(true){
        std::cout << "Provide commands: ";
        getline(std::cin , input);

        if(input.compare("exit") == 0){
            break;
        }

        Tokenizer token(input);    

        // Create pipe
        // pipe(fd);

        for (size_t i = 0; i < token.commands.size(); i++){

            for(size_t j = 0; j < token.commands[i]->args.size(); j++){
                arg.push_back(&token.commands[i]->args[j][0]);
            }   
            arg.push_back(NULL);
            pipe(fd);

            // Create child to run first command
            pid = fork();

            if (pid == 0){
                // In child, redirect output to write end of pipe
                if(i < token.commands.size()-1){
                    dup2(fd[1], 1);
                }
                // Close the read end of the pipe on the child side.
                close(fd[0]);
                // In child, execute the command
                execvp(arg[0], arg.data());

                return(0);
            } else {
                 //redirect the SHELL(PARENT)'s input to the read end of the pipe
                dup2(fd[0], 0);
                //closes the write end of the pipe on the parent side
                close(fd[1]);
                close(fd[0]);
                //wait until the last command finishes
                // waitpid(pid, &status, 0);
                // wait(0);
                if (i == token.commands.size()-1){
                    wait(0);
                }
                
            }
            
            arg.clear();
        }

        //restore the stdin and stdout
        // Reset the input and output file descriptors of the parent.
        // overwrite in/out with what was saved
        dup2(stdout, 1);
        dup2(stdin, 0);
    }    
}
