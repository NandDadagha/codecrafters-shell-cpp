#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>    // getenv
#include <sstream>    // stringstream
#include <unistd.h>   // access
#include <sys/stat.h> // stat
#include <unistd.h>   // fork, execvp
#include <sys/wait.h> // wait
#include <cctype>

bool isExecutable(const std::string &path)
{
  struct stat sb;
  return stat(path.c_str(), &sb) == 0 &&
         S_ISREG(sb.st_mode) &&
         access(path.c_str(), X_OK) == 0;
}

std::vector<std::string> parsing(const std::string &input)
{
  std::vector<std::string> tokens;
  std::string curr_token = "";
  bool in_single_quotes = false;
  bool in_double_quotes = false;
  for(char c : input) {
    if(c == '\"' && in_double_quotes) {
      in_double_quotes = false;
    }
    else if(c == '\'' && in_single_quotes) {
      in_single_quotes = false;
    }
    else if(c == '\'' && !in_double_quotes) {
      in_single_quotes = true;
    }
    else if(c == '\"' && !in_single_quotes) {
      in_double_quotes = true;
    }
    else if(std::isspace(c) && !in_single_quotes && !in_double_quotes) {
      if(!curr_token.empty()) {
        tokens.push_back(curr_token);
        curr_token.clear();
      }
    }
    else {
      curr_token += c;
    }
  }
  if(!curr_token.empty()) {
    tokens.push_back(curr_token);
  }
  return tokens;
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
    std::string command = tokens[0];

    // exit
    if (command == "exit")
      exit(0);

    // echo
    else if (command == "echo")
    {
      for (size_t i = 1; i < tokens.size(); i++)
      {
        std::cout << tokens[i] << (i == tokens.size() - 1 ? "" : " ");
      }
      std::cout << std::endl;
      continue;
    }
    // pwd
    else if (command == "pwd")
    {
      char buffer[1024];
      if (getcwd(buffer, sizeof(buffer)) != nullptr)
      {
        std::cout << buffer << "\n";
      }
      continue;
    }
    // cd
    else if (command == "cd")
    {
      if (tokens.size() < 2 || tokens[1] == "~")
      {
        const char *homeEnv = getenv("HOME");
        if (chdir(homeEnv) != 0)
        {
          std::cout << "cd: " << tokens[1] << ": No such file or directory\n";
        }
        continue;
      }
      else if (chdir(tokens[1].c_str()) != 0)
      {
        std::cout << "cd: " << tokens[1] << ": No such file or directory\n";
      }
      continue;
    }
    // type
    else if (command == "type")
    {
      if (tokens.size() < 2)
        continue; // Handle case where user just types 'type' without argv
      std::string target = tokens[1];
      if (target == "echo" || target == "exit" || target == "type" || target == "pwd" || target == "cd")
      {
        std::cout << target << " is a shell builtin\n";
        continue;
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
        continue;
      }
    }
    // execute
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
      for (auto &s : tokens)
      {

        argv.push_back(&s[0]);
      }
      argv.push_back(nullptr);

      pid_t pid = fork();
      if (pid == 0)
      {
        // child process
        if (execv(executablePath.c_str(), argv.data()) == -1)
          exit(1);
      }
      else if (pid > 0)
      { // parent process
        int status;
        waitpid(pid, &status, 0);
      }
      continue;
    }
    std::cout << input << ": command not found" << std::endl;
  }
}
