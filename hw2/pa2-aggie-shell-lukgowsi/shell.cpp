#include <iostream>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <fstream>
#include <vector>
#include <string>

#include "Tokenizer.h"

// all the basic colours for a shell prompt
#define RED     "\033[1;31m"
#define GREEN	"\033[1;32m"
#define YELLOW  "\033[1;33m"
#define BLUE	"\033[1;34m"
#define WHITE	"\033[1;37m"
#define NC      "\033[0m"

using namespace std;

int main () {
    // create copies of stdin / stdout; dup()
    // save original input and output
    int stdout = dup(1);
    int stdin = dup(0);

    pid_t pid;
    int fd[2];

    vector <char*> arg;
    char prev[1024];
    char curr[1024];

    int status;
    int stat; 
    std::vector <int> children;


    for (;;) {
        // implement iterator over vector of bg pid (vector also declared outisde loop)
        // waitpid() - using flag for non-blocking
        for(size_t k = 0; k < children.size(); k++){
            stat = waitpid(pid, &status, WNOHANG);

            if(stat > 0){
                children.erase(children.begin() + k);
            }
        }

        // implement date / time with TODO
        // implement username with getlogin()
        // implement curdir with getcwd()
        time_t currTime = time(NULL); // gives current time
        string timeStr = ctime(&currTime); // changes time into string
        char* user = getenv("USER");
        getcwd(curr, 1024);

        string tim = timeStr.substr(4,5);

        string out = tim + " " + user + ":" + curr + "$";

        // need date/time, username, and absolute path to current dir
        cout << YELLOW << out << NC << " ";
        
        // get user inputted command
        string input;
        getline(cin, input);

        if (input == "exit") {  // print exit message and break out of infinite loop
            cout << RED << "Now exiting shell..." << endl << "Goodbye" << NC << endl;
            break;
        }

        // implement cd with chdir()
        // if dir (cd <dir>) is "-", then go to previous working directory
        // variable stroing previous working directory (it needs to be declared outside loop)
        if(input.substr(0,2) == "cd"){
            if(input.substr(3) == "-"){
                chdir(prev);
                continue;
            }

            getcwd(prev, 1024);
            chdir(input.substr(3).c_str());
            continue;
        }

        // get tokenized commands from user input
        Tokenizer tknr(input);
        if (tknr.hasError()) {  // continue to next prompt if input had an error
            continue;
        }

        // print out every command token-by-token on individual lines
        // prints to cerr to avoid influencing autograder
        for (auto cmd : tknr.commands) {
            for (auto str : cmd->args) {
                cerr << "|" << str << "| ";
            }
            if (cmd->hasInput()) {
                cerr << "in< " << cmd->in_file << " ";
            }
            if (cmd->hasOutput()) {
                cerr << "out> " << cmd->out_file << " ";
            }
            cerr << endl;
        }

        // for piping
        // for cmd : commands
        //      call pipe() to make pipe
        //      fork() - in child, redirect stdout; in par, redirect stdin
        //      ^ is already written (yay)
        // add checks for first / last command 

        // adds check for bg process - add pid to vector if bg and don't waitpid in par
        for (size_t i = 0; i < tknr.commands.size(); i++){
            for(size_t j = 0; j < tknr.commands[i]->args.size(); j++){
                arg.push_back(&tknr.commands[i]->args[j][0]);
            }   
            arg.push_back(NULL);
            pipe(fd);

            // Create child to run first command
            pid = fork();

            if (pid == 0){
                if(tknr.commands[i]->hasOutput()){
                    FILE *inFile = fopen(tknr.commands[i]->out_file.c_str(), "w");
                    dup2(fileno(inFile), 1);
                    close(fileno(inFile));
                }

                if(tknr.commands[i]->hasInput()){
                    FILE *outFile = fopen(tknr.commands[i]->in_file.c_str(), "r");
                    dup2(fileno(outFile), 0);
                    close(fileno(outFile));
                }

                // In child, redirect output to write end of pipe
                if(i < tknr.commands.size()-1){
                    dup2(fd[1], 1);
                }
                // Close the read end of the pipe on the child side.
                close(fd[0]);
                // In child, execute the command

                execvp(arg[0], arg.data());

                return 0;
            } else {
                 //redirect the SHELL(PARENT)'s input to the read end of the pipe
                dup2(fd[0], 0);
                //closes the write end of the pipe on the parent side
                close(fd[1]);
                close(fd[0]);

                if(tknr.commands[i]->isBackground()){
                    children.push_back(pid); //puts pid into vector if background
                    continue;
                } else {
                    waitpid(pid, &status, WUNTRACED); //waits for child to be killed or stopped
                }
            }
            
            arg.clear();
        }

        dup2(stdout, 1);
        dup2(stdin, 0);
    }
}
