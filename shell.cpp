#include <iostream>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>

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
    //TODO cwd and pwd
    char cwd[256];
    char pwd[256];
    //TODO create copies of stdin and stdout ; dup() to create those copies
    //save original stdin and stdout
    int saved_stdin = dup(0);
    int saved_stdout = dup(1);
    //vector
    vector<Command> _background;

    for (;;) {
        //TODO implement iteration over vector of bg pid (vector also declared outside this loop)
        //implement date/time with TODO
        time_t _time = time(nullptr);
        char* _date = ctime(&_time);

        //implement username with getlogin()
        string username = getenv("USER");

        //implement current directory with getcwd() 
        getcwd(cwd, sizeof(cwd));

        //take out new line on time
        _date[strlen(_date) - 1] = '\0';

        // need date/time, username, and absolute path to current dir
        cout << YELLOW << _date << " " << username << ":" << cwd << "$" << NC << " ";
        
        // get user inputted command
        string input;
        getline(cin, input);

        if(input == "exit") {  // print exit message and break out of infinite loop
            cout << RED << "Now exiting shell..." << endl << "Goodbye" << NC << endl;
            break;
        }
        
        // get tokenized commands from user input
        Tokenizer token(input);
        if (token.hasError()) {  // continue to next prompt if input had an error
            continue;
        }

        //handle directory processing
        if(token.commands.at(0)->args.at(0) == "cd"){
            if(token.commands.at(0)->args.at(1) == "-"){
                //change path to past directory
                chdir(pwd);
            }
            else{
                //create new pathway
                string path_way = token.commands.at(0)->args.at(1); //path_way is whatever file we want to access
                strcpy(pwd, cwd); //past becomes copy of current
                chdir(path_way.c_str()); //set new pathway as current directory
            }

            continue;
        }

        //LE2
        // TODO: add functionality
        //foreach command in tokens.commands
        for(unsigned int i = 0; i < token.commands.size(); i++){
            // Create pipe
            int pipes[2];
            pipe(pipes); 

            // Create child to run first command
            pid_t p = fork();
            if(p == 0){
                // In child, redirect output to write end of pipe
                if(i < token.commands.size()-1){
                    dup2(pipes[1], 1);
                    close(pipes[0]);
                }
                
                //check for input in command line
                if(token.commands.at(i)->hasInput()){
                    //open file
                    int in = open(token.commands.at(i)->in_file.c_str(), O_RDONLY, S_IRWXU);
                    //redirect input using dup2 to pipe
                    dup2(in, 0);
                }

                //check for output in command line
                if(token.commands.at(i)->hasOutput()){
                    //open file
                    int out = creat(token.commands.at(i)->out_file.c_str(), S_IRWXU);
                    //redirect output using dup2 to pipe 
                    dup2(out, 1);
                }

                char** cmd1 = new char*[token.commands.at(i)->args.size()+1];
                //submit inputs into cmd1
                for(unsigned int j = 0; j < token.commands.at(i)->args.size(); j++){
                    cmd1[j] = {(char*) token.commands.at(i)->args.at(j).c_str()};
                }

                cmd1[token.commands.at(i)->args.size()] = nullptr;

                // In child, execute the command   
                execvp(*cmd1, cmd1);
                delete[] cmd1;
            }
            else{
                //if cmd bg is true
                if(token.commands.at(i)->isBackground()){
                    //push onto your vector
                    _background.push_back(*token.commands.at(i));
                }
                else if(i == token.commands.size()-1){
                    int status = 0;
                    waitpid(p, &status, 0);
                }

                if(token.commands.size() > 1){
                    //redirect input to read end of parent's pipe
                    dup2(pipes[0], 0);
                    //close write end of pipe
                    close(pipes[1]);
                }
            }

            //close files
            close(pipes[0]);
            close(pipes[1]);
        }

        //TODO restore stdin and stdout outside the loop
        //restore the stdin and stdout
        dup2(saved_stdin, 0);
        dup2(saved_stdout, 1);
    }
}
