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

// 19/01/2026 : Process Groups, Ctrl + Z, fg, bg, and terminal control



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

    // Shell must be its own Process Group
    shell_pgid = getpid();
    
    // Assigns process pid to process group pgid
    // pgid == 0, use pid as pgid, i.e this is the first process of that
    // group
    setpgid(shell_pgid, shell_pgid);

    // Terminal Control, given to a particular pgid
    // Means : Send keyboard-generated signals to this PGID
    // Why? Ctrl + C -> SIGINT, the kernel sends signals to foreground pgid
    // Not to the complete shell, i.e Ctrl + C doesn't kill Astra now

    // Terminal Behaviour : 
    // Thus Shell Idle     : Shell PGID
    //      Foreground Job : Job PGID
    //      Background Job : No Terminal

    tcsetpgrp(STDIN_FILENO, shell_pgid);

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

    // Bring a job to foreground, give terminal to job
    // Resume job if stopped, wait for it and take terminal back
    if (tokens[0] == "fg") {
        int job_id = std::stoi(tokens[1].substr(1));

        for (auto it = jobs_.begin(); it != jobs_.end(); ++it) {
            if (it->job_id == job_id) {
                // Terminal now sends singals to job
                tcsetpgrp(STDIN_FILENO, it->pgid);
                // -pgid -> signal entire process group, resume every process in the job
                kill(-it->pgid, SIGCONT);

                int status;
                // Why use "-" sign? Syntactically it means, 'Wait for any member of this job'
                // Wait for any process in the group, return if stopped (Ctrl + Z)
                waitpid(-it->pgid, &status, WUNTRACED);

                // Shell shall regain terminal ownership
                tcsetpgrp(STDIN_FILENO, shell_pgid);

                if (WIFSTOPPED(status)) {
                    it->state = JobState::Stopped;
                } else {
                    jobs_.erase(it);
                }
                return;
            }
        }
    }

    // Resume in Background, do Not give terminal to job, unlike fg
    if (tokens[0] == "bg") {
        
        int job_id = std::stoi(tokens[1].substr(1));

        for (auto& job : jobs_) {
            if (job.job_id == job_id) {
                // No tcsetpgrp(), so terminal stays with shell, job runs in background
                kill(-job.pgid, SIGCONT);
                job.state = JobState::Running;

                std::cout << "[" << job.job_id << "] "
                    << job.command << " &" << std::endl;
                
                return;
            }
        }
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
        // new job, new group, this pgid == 0 if pid == 0
        setpgid(0, 0);

        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGCONT, SIG_DFL);

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

        // parent and child run concurrently, either may reach setpgid first
        // calling in both places ensures correctness (parent and child)

        setpgid(pid, pid);

        // If parent process and the job is a background process
        if (background) {
            jobs_.push_back(Job{
                next_job_id_++,
                pid,
                line,
                JobState::Running
            });

            std::cout << "[" << jobs_.back().job_id << "] " << pid << std::endl;
        }
        else{

            tcsetpgrp(STDIN_FILENO, pid);

            int status;
            waitpid(-pid, &status, WUNTRACED);

            tcsetpgrp(STDIN_FILENO, shell_pgid);

            if (WIFSTOPPED(status)) {
                jobs_.push_back(Job{
                    next_job_id_++,
                    pid,
                    line,
                    JobState::Stopped
                });
                std::cout << "[" << jobs_.back().job_id
                    << "] Stopped " << line << std::endl;
            }
            // pid doesnot exist anymore
            // waitpid(pid, nullptr, 0);
        }
    }

    else{
        perror("fork");
    }
} 

void Shell::check_background_jobs() {
    for (auto it = jobs_.begin(); it != jobs_.end(); ) {
        pid_t result = waitpid(-it->pgid, nullptr, WNOHANG);

        if (result > 0 && it->state == JobState::Running) {
            std::cout << "[" << it->job_id << "] Done  "
                      << it->command << std::endl;
            it = jobs_.erase(it);
        } 
        else
            ++it;
    }
}