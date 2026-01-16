#pragma once
#include <string>

class Shell{
    public :
        void run();
    private:
        void print_prompt() const;
        std::string read_line() const;
        void execute_line(const std::string& line);
};