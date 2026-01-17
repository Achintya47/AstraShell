#include "shell.hpp"

#include <iostream>
#include <sstream>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <limits.h>
#include <errno.h>


// 18/01/2026 : Implement Built-In Commands
//              Background processes '&'
//              Background process completion and handling
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

/**
 * @brief Why are we reaping or handling the forked children?
 * We're in a way collecting the death certificates of the dead
 * children processes, in order for our system to re-allocate the pid's.
 * Too many zombie processes : fork() starts failing, reap == cleanup
 * 
 * But they say, "Signal handlers are restricted", why so?
 * Because a signal handler can interrupt your program at any instruction
 * ANYWHERE!, halfway through malloc, writing to cout, inside new anything
 * Thus non-re-entrant functions, will be corrupted.
 * 
 * Malloc is allocating, handler allocates memory and malloc runs again while already
 * allocating : heap corrpution
 * 
 * cout uses internal buffers and locks, an interrupt could cause corrupted output, 
 * a potential DEADLOCK.
 * Scary stuff mate!!
 * 
 * Why waitpid is allowed? POSIX gurantees that waitpid() is async-signal-safe
 * 
 */
void sigchld_handler(int) {

    // Do nothing as of now
    
    // // Save errno, to restore later
    // int saved_errno = errno;

    // // We loop to reap all finished children
    // while(true) {
    //     // Wiat for any child, WNOHANG -> Donot block
    //     // >0 Reaped one child, 0 No child ready, -1 no child exists
    //     pid_t pid = waitpid(-1, nullptr, WNOHANG);
    //     if (pid <= 0){
    //         break;
    //     }
    // }
    // // restore later
    // errno = saved_errno;
}

void Shell::run() {
    
    // Ignore interactive signals in the shell
    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);

    signal(SIGCHLD, sigchld_handler);

    while(true) {
        print_prompt();

        std::string line = read_line();
        if (line.empty())
            continue;
        if (line == "exit")
            break;
        
        execute_line(line);
        check_background_jobs();
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

    if (tokens[0] == "jobs") {
        for (const auto& job : jobs_) {
            std::cout << "[" << job.job_id << "] "
                << "Running "
                << job.command << std::endl;
        }
        return; 
    }


    
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
            jobs_.push_back(Job{
                next_job_id_++,
                pid,
                line
            });

            std::cout << "[" << jobs_.back().job_id << "] " << pid << std::endl;
        }
        else{
            waitpid(pid, nullptr, 0);
        }
    }
    else{
        perror("fork");
    }
} 

void Shell::check_background_jobs() {
    for (auto it = jobs_.begin(); it != jobs_.end(); ) {
        pid_t pid = waitpid(it->pid, nullptr, WNOHANG);

        if (pid > 0) {
            std::cout << "[" << it->job_id << "] Done  "
                      << it->command << std::endl;
            it = jobs_.erase(it);
        } 
        else
            ++it;
    }
}