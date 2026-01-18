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

        std::vector<Job> jobs_;
        int next_job_id_ = 1;
};