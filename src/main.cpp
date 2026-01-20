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

bool isExecutable(const std::string &path)
{
  struct stat sb;
  return stat(path.c_str(), &sb) == 0 &&
         S_ISREG(sb.st_mode) &&
         access(path.c_str(), X_OK) == 0;
}

// PARSING
std::vector<std::string> parsing(const std::string &input)
{
  std::vector<std::string> filteredToken;
  std::string curr_token = "";
  bool in_single_quotes = false;
  bool in_double_quotes = false;
  for (size_t i = 0; i < input.size(); i++)
  {
    if (input[i] == '\\' && in_double_quotes)
    {
      if (input[i + 1] == '\\' || input[i + 1] == '"')
      {
        curr_token += input[i + 1];
        i++;
      }
      else
      {
        curr_token += input[i];
      }
    }
    else if (input[i] == '\\' && !in_double_quotes && !in_single_quotes)
    {
      if (i + 1 < input.size())
      {
        curr_token += input[i + 1];
        i++;
      }
    }
    else if (input[i] == '\"' && in_double_quotes)
    {
      in_double_quotes = false;
    }
    else if (input[i] == '\'' && in_single_quotes)
    {
      in_single_quotes = false;
    }
    else if (input[i] == '\'' && !in_double_quotes)
    {
      in_single_quotes = true;
    }
    else if (input[i] == '\"' && !in_single_quotes)
    {
      in_double_quotes = true;
    }
    else if (std::isspace(input[i]) && !in_single_quotes && !in_double_quotes)
    {
      if (!curr_token.empty())
      {
        filteredToken.push_back(curr_token);
        curr_token.clear();
      }
    }
    else
    {
      curr_token += input[i];
    }
  }
  if (!curr_token.empty())
  {
    filteredToken.push_back(curr_token);
  }
  return filteredToken;
}

int main()
{
  // Flush after every std::std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  while (1)
  {
    std::cout << "$ ";
    std::string input;
    if (!getline(std::cin, input)) // ctrl + d to exit shell
    {
      std::cout << "\n";
      break;
    }
    if (input.empty()) // pressing "enter" wh blank line
      continue;

    std::vector<std::string> tokens = parsing(input);

    // Redirection
    std::string stdoutFile = "";
    std::string stderrFile = "";
    bool redirectStdout = false;
    bool redirectStderr = false;
    std::vector<std::string> filteredToken;
    for (size_t i = 0; i < tokens.size(); i++)
    {
      if ((tokens[i] == ">" || tokens[i] == "1>") && i + 1 < tokens.size())
      {
        redirectStdout = true;
        stdoutFile = tokens[i + 1];
        i++;
      }
      else if (tokens[i] == "2>" && i + 1 < tokens.size())
      {
        redirectStderr = true;
        stderrFile = tokens[i + 1];
        i++;
      }
      else
      {
        filteredToken.push_back(tokens[i]);
      }
    }
    if (filteredToken.empty())
      continue;

    std::string command = filteredToken[0];

    // redirecting stdout
    int original_stdout = -1;
    if (redirectStdout)
    {
      if (command == "echo" || command == "pwd" || command == "type")
      {
        original_stdout = dup(STDOUT_FILENO); // backup terminal
        int fd = open(stdoutFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0664);
        if (fd != -1)
        {
          dup2(fd, STDOUT_FILENO);
          close(fd);
        }
      }
    }
    // redirecting stderr
    int original_stderr = -1;
    if (redirectStderr)
    {
      if (command == "echo" || command == "pwd" || command == "type")
      {
        original_stderr = dup(STDERR_FILENO); // backup
        int fderr = open(stderrFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0664);
        if (fderr != 1)
        {
          dup2(fderr, STDERR_FILENO);
          close(fderr);
        }
      }
    }
    // BUILT-IN
    // exit
    if (command == "exit")
      exit(0);

    // echo
    else if (command == "echo")
    {
      for (size_t i = 1; i < filteredToken.size(); i++)
      {
        std::cout << filteredToken[i] << (i == filteredToken.size() - 1 ? "" : " ");
      }
      std::cout << std::endl;
    }
    // pwd
    else if (command == "pwd")
    {
      char buffer[1024];
      if (getcwd(buffer, sizeof(buffer)) != nullptr)
      {
        std::cout << buffer << "\n";
      }
    }
    // cd
    else if (command == "cd")
    {
      if (filteredToken.size() < 2 || filteredToken[1] == "~")
      {
        const char *homeEnv = getenv("HOME");
        if (chdir(homeEnv) != 0)
        {
          std::cout << "cd: " << filteredToken[1] << ": No such file or directory\n";
        }
      }
      else if (chdir(filteredToken[1].c_str()) != 0)
      {
        std::cout << "cd: " << filteredToken[1] << ": No such file or directory\n";
      }
    }
    // type
    else if (command == "type")
    {
      if (filteredToken.size() < 2)
        continue; // Handle case where user just types 'type' without argv
      std::string target = filteredToken[1];
      if (target == "echo" || target == "exit" || target == "type" || target == "pwd" || target == "cd")
      {
        std::cout << target << " is a shell builtin\n";
      }
      else
      {
        const char *pathEnv = getenv("PATH");
        if (!pathEnv)
        {
          std::cout << target << ": not found\n";
          continue;
        }
        std::stringstream ss(pathEnv);
        std::string dir;
        bool flag = false;
        while (getline(ss, dir, ':'))
        {
          std::string fullPath = dir + "/" + target;
          // is it executable?
          if (isExecutable(fullPath))
          {
            std::cout << target << " is " << fullPath << "\n";
            flag = true;
            break;
          }
        }
        if (!flag)
        {
          std::cout << target << ": not found\n";
        }
      }
    }
    // execute
    else
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
        for (auto &s : filteredToken)
        {
          argv.push_back(&s[0]);
        }
        argv.push_back(nullptr);

        pid_t pid = fork();
        if (pid == 0) // child process
        {
          if (redirectStdout)
          {
            int fd = open(stdoutFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0664);
            if (fd == -1)
            {
              exit(1);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
          }
          if (redirectStderr)
          {
            int fderr = open(stderrFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0664);
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
}
