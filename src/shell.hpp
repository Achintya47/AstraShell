#pragma once
#include <string>
#include <vector>

enum class JobState {
    Running,
    Stopped
};

struct Job {
    int job_id;
    pid_t pgid;
    std::string command;
    JobState state;
};

class Shell{
    public :
        void run();
    private:
        void print_prompt() const;
        std::string read_line() const;
        void execute_line(const std::string& line);
        void check_background_jobs();
        void execute_pipeline(const std::vector<std::string>& tokens, bool background);
        std::string join_tokens(const std::vector<std::string>& tokens);


        // Process group is a set of related processes treated as one job
        pid_t shell_pgid; // For global acess
        std::vector<Job> jobs_;
        int next_job_id_ = 1;
};