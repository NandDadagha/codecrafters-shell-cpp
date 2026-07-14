#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>    // getenv
#include <sstream>    // stringstream
#include <unistd.h>   // access, fork, execvp
#include <sys/stat.h> // stat
#include <fcntl.h>    // open
#include <sys/wait.h> // wait
#include <cctype>
#include <readline/readline.h> // readline()
#include <readline/history.h>
#include <dirent.h> // readdir, opendir, DIR*
#include "parser.h"
#include "utils.h"
#include "builtins.h"
#include "completion.h"
#include "command.h"
#include "redirection.h"

int main()
{
  // Flush after every std::std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  int session_start_index = history_length;
  const char *hist_path = getenv("HISTFILE");
  if (hist_path == nullptr)
    hist_path = ".my_shell_history";
  read_history(hist_path);
  while (1)
  {
    rl_attempted_completion_function = completion_function;
    char *line = readline("$ ");
    if (!line) // EOF (Ctrl + D)
    {
      std::cout << "\n";
      break;
    }
    std::string input;
    if (line)
    {
      input = line;
      free(line);
    }
    if (input.empty()) // pressing "enter" blank line
      continue;

    add_history(input.c_str());

    std::vector<std::string> tokens = parse(input);

    // Redirection
    Command commandData;
    commandData.args = tokens;

    parseRedirection(commandData);

    if (commandData.args.empty())
      continue;

    std::string command = commandData.args[0];

    // redirecting stdout
    int original_stdout = -1;
    if (commandData.redirectStdout)
    {
      if (command == "echo" || command == "pwd" || command == "type" || command == "history")
      {
        original_stdout = dup(STDOUT_FILENO); // backup terminal
        int flags = O_WRONLY | O_CREAT;
        if (!commandData.redirectStdout)
          flags |= O_TRUNC;
        else
          flags |= O_APPEND;
        int fd = open(commandData.stdoutFile.c_str(), flags, 0664);
        if (fd != -1)
        {
          dup2(fd, STDOUT_FILENO);
          close(fd);
        }
      }
    }
    // redirecting stderr
    int original_stderr = -1;
    if (commandData.redirectStderr)
    {
      if (command == "echo" || command == "pwd" || command == "type" || command == "history")
      {
        original_stderr = dup(STDERR_FILENO); // backup
        int flags = O_CREAT | O_WRONLY;
        if (commandData.appendStderr)
          flags |= O_APPEND;
        else
          flags |= O_TRUNC;
        int fderr = open(commandData.stderrFile.c_str(), flags, 0664);
        if (fderr != 1)
        {
          dup2(fderr, STDERR_FILENO);
          close(fderr);
        }
      }
    }
    // Piping
    std::vector<std::vector<std::string>> commands;
    std::vector<std::string> current_cmd;
    bool hasPipe = false;

    for (const auto &token : commandData.args)
    {
      if (token == "|")
      {
        hasPipe = true;
        if (!current_cmd.empty())
        {
          commands.push_back(current_cmd);
          current_cmd.clear();
        }
      }
      else
      {
        current_cmd.push_back(token);
      }
    }
    if (!current_cmd.empty())
      commands.push_back(current_cmd);

    // 2. Execution Loop
    if (hasPipe && commands.size() > 1)
    {
      std::vector<pid_t> pids;
      int prev_read = -1;
      int pidfds[2];

      for (size_t i = 0; i < commands.size(); i++)
      {
        // Only create a pipe if we are NOT the last command
        if (i < commands.size() - 1)
        {
          if (pipe(pidfds) == -1) {
            perror("pipe");
            break;
          }
        }

        pid_t pid = fork();
        if (pid == 0) // CHILD
        {
          // A. Input Wiring (Read from left)
          if (prev_read != -1)
          {
            dup2(prev_read, STDIN_FILENO);
            close(prev_read);
          }

          // B. Output Wiring (Write to right)
          if (i < commands.size() - 1)
          {
            dup2(pidfds[1], STDOUT_FILENO);
            close(pidfds[1]); 
            close(pidfds[0]); // Child doesn't need the read end of its own output pipe
          }

          // C. Execute
          if (handle_builtin(commands[i], hist_path, session_start_index))
            exit(0);

          std::vector<char *> argv;
          for (auto &s : commands[i])
            argv.push_back(&s[0]);
          argv.push_back(nullptr);

          execvp(argv[0], argv.data());
          perror("execvp"); // Good for debugging
          exit(1);
        }
        else if (pid > 0) // PARENT
        {
          pids.push_back(pid);
          
          // Close the read-end we just handed off to the child
          if (prev_read != -1)
            close(prev_read);

          // Setup for next iteration
          if (i < commands.size() - 1)
          {
            close(pidfds[1]);      // Parent MUST close write end
            prev_read = pidfds[0]; // Save read end for the next child
          }
        }
      }
      for (pid_t p : pids)
      {
        waitpid(p, nullptr, 0);
      }
      continue;
    }
    // execute
    if (!handle_builtin(commandData.args, hist_path, session_start_index))
    {
      {
        const char *pathEnv = getenv("PATH");
        std::stringstream ss(pathEnv ? pathEnv : "");
        std::string dir;
        std::string executablePath = "";
        while (getline(ss, dir, ':'))
        {
          std::string fullPath = dir + "/" + command;
          if (isExecutable(fullPath))
          {
            executablePath = fullPath;
            break;
          }
        }
        if (!executablePath.empty())
        {
          std::vector<char *> argv; // char* instead of string because kernal can't understand c++
          for (auto &s : commandData.args)
          {
            argv.push_back(&s[0]);
          }
          argv.push_back(nullptr);

          pid_t pid = fork();
          if (pid == 0) // child process
          {
            if (commandData.redirectStdout)
            {
              int flags = O_CREAT | O_WRONLY;
              if (!commandData.appendStdout)
                flags |= O_TRUNC;
              else
                flags |= O_APPEND;
              int fd = open(commandData.stdoutFile.c_str(), flags, 0664);
              if (fd == -1)
              {
                exit(1);
              }
              dup2(fd, STDOUT_FILENO);
              close(fd);
            }
            if (commandData.redirectStderr)
            {
              int flags = O_CREAT | O_WRONLY;
              if (!commandData.appendStderr)
                flags |= O_TRUNC;
              else
                flags |= O_APPEND;
              int fderr = open(commandData.stderrFile.c_str(), flags, 0664);
              if (fderr == -1)
              {
                exit(1);
              }
              dup2(fderr, STDERR_FILENO);
              close(fderr);
            }
            if (execv(executablePath.c_str(), argv.data()) == -1)
              exit(1);
          }
          else if (pid > 0) // parent process
          {
            int status;
            waitpid(pid, &status, 0);
          }
        }
        else
        {
          std::cout << input << ": command not found" << std::endl;
        }
      }
    }
    if (original_stdout != -1)
    {
      dup2(original_stdout, STDOUT_FILENO);
      close(original_stdout);
    }
    if (original_stderr != -1)
    {
      dup2(original_stderr, STDERR_FILENO);
      close(original_stderr);
    }
  }
  write_history(hist_path);
}
