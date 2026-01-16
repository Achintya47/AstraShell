#include "shell.hpp"

#include <iostream>
#include <sstream>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>

namespace {
    std:: vector<std::string> split(const std::string& line){
        std::istringstream iss(line);
        std::vector<std::string> tokens;
        std::string token;

        while (iss >> token)
            tokens.push_back(token);
        
            return tokens;
    }
}

void Shell::run() {
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
    
    std::vector<char*> argv;
    for (auto& s : tokens)
        argv.push_back(const_cast<char*>(s.c_str()));
    
    argv.push_back(nullptr);

    pid_t pid = fork();

    if (pid == 0) {
        execvp(argv[0], argv.data());
        perror("execvp");
        _exit(1);
    }
    else if(pid > 0) {
        waitpid(pid, nullptr, 0);
    }
    else{
        perror("fork");
    }
} 