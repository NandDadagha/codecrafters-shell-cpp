#include "redirection.h"

void parseRedirection(Command &command)
{
    std::vector<std::string> filtered;

    for (size_t i = 0; i < command.args.size(); i++)
    {
        if ((command.args[i] == ">>" || command.args[i] == "1>>") &&
            i + 1 < command.args.size())
        {
            command.redirectStdout = true;
            command.appendStdout = true;
            command.stdoutFile = command.args[i + 1];
            i++;
        }
        else if (command.args[i] == "2>>" &&
                 i + 1 < command.args.size())
        {
            command.redirectStderr = true;
            command.appendStderr = true;
            command.stderrFile = command.args[i + 1];
            i++;
        }
        else if ((command.args[i] == ">" || command.args[i] == "1>") &&
                 i + 1 < command.args.size())
        {
            command.redirectStdout = true;
            command.stdoutFile = command.args[i + 1];
            i++;
        }
        else if (command.args[i] == "2>" &&
                 i + 1 < command.args.size())
        {
            command.redirectStderr = true;
            command.stderrFile = command.args[i + 1];
            i++;
        }
        else
        {
            filtered.push_back(command.args[i]);
        }
    }

    command.args = filtered;
}
