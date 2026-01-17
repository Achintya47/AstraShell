#include "shell.hpp"

#include <iostream>
#include <sstream>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <limits.h>


// 18/01/2026 : Implement Built-In Commands
//              Background processes '&'
//


namespace {
    // Private anonymous namespace, restricted to shell.cpp
    std:: vector<std::string> split(const std::string& line){

        // istringstream treats the string as a stream interface,
        // thus we can treat it as a stream and use the cin >> x -> iss >> x;
        std::istringstream iss(line);
        std::vector<std::string> tokens;
        std::string token;

        // thus we are extracting tokens while minimizing overhead and bugs
        while (iss >> token)
            tokens.push_back(token);
        
            return tokens;
    }
}

void Shell::run() {
    
    // Ignore interactive signals in the shell
    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);

    while(true) {
        print_prompt();

        std::string line = read_line();
        if (line.empty())
            continue;
        if (line == "exit")
            break;
        
        execute_line(line);
    }
}

void Shell::print_prompt() const {
    std::cout << "astra$ " << std::flush;
}

std::string Shell::read_line() const {
    std::string line;
    std::getline(std::cin, line);
    return line;
}

void Shell::execute_line(const std::string &line) {
    auto tokens = split(line);
    if (tokens.empty())
        return;
    
    bool background = false;

    if (tokens.back() == "&") {
        background = true;
        tokens.pop_back(); // remove '&'
    }
    
    /* BUILT INS */
    // Checking for Built-In commands to avoid fork
    if (tokens[0] == "cd") {
        const char * path = 
            (tokens.size() > 1) ? tokens[1].c_str() : getenv("HOME");
        
        if (chdir(path) != 0) {
            perror("cd");
        }

        return;
    }
    
    if (tokens[0] == "pwd") {
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd))) {
            std::cout << cwd << std::endl;
        }
        else {
            perror("pwd");
        }

        return;
    }

    std::vector<char*> argv;
    for (auto& s : tokens)
        argv.push_back(const_cast<char*>(s.c_str()));
    
    argv.push_back(nullptr);

    pid_t pid = fork();

    if (pid == 0) {
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);

        // If execvp succeeds, child terminates then and there, thus exit not called
        execvp(argv[0], argv.data());
        
        // system calls donot throw exceptions, thus perror() captures the errno
        // and prints a human-readable message
        perror("execvp");
        // _empty() performs kernel level exit without flushing buffers, destructor calls
        // doesn't call atexit() handler
        // Parent and Child share buffer, thus exit() can cause flush twice
        _exit(1);
    }

    else if(pid > 0) {
        // If parent process and the job is a background process
        if (background) {
            static int job_id = 1;
            std::cout << "[" << job_id++ << "] " << pid << std::endl;
        }
        else{
        waitpid(pid, nullptr, 0);
        }
    }
    else{
        perror("fork");
    }
} 