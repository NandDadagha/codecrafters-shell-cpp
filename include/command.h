#ifndef COMMAND_H
#define COMMAND_H

#include <string>
#include <vector>

struct Command
{
    std::vector<std::string> args;

    bool redirectStdout = false;
    bool redirectStderr = false;

    bool appendStdout = false;
    bool appendStderr = false;

    std::string stdoutFile;
    std::string stderrFile;
};

#endif