#include "builtins.h"
#include "utils.h"

#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <unistd.h>

#include <readline/readline.h>
#include <readline/history.h>

bool handle_builtin(const std::vector<std::string> &filteredToken, const char *hist_path, int &session_start_index)
{
  // BUILT-IN
  // exit
  std::string command = filteredToken[0];
  if (command == "exit")
  {
    write_history(hist_path);
    exit(0);
  }
  // echo
  else if (command == "echo")
  {
    for (size_t i = 1; i < filteredToken.size(); i++)
    {
      std::cout << filteredToken[i] << (i == filteredToken.size() - 1 ? "" : " ");
    }
    std::cout << std::endl;
    return true;
  }
  // pwd
  else if (command == "pwd")
  {
    char buffer[1024];
    if (getcwd(buffer, sizeof(buffer)) != nullptr)
    {
      std::cout << buffer << "\n";
    }
    return true;
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
    return true;
  }
  // type
  else if (command == "type")
  {
    if (filteredToken.size() < 2)
      return true;
    std::string target = filteredToken[1];
    if (target == "echo" || target == "exit" || target == "type" || target == "pwd" || target == "cd" || target == "history")
    {
      std::cout << target << " is a shell builtin\n";
    }
    else
    {
      const char *pathEnv = getenv("PATH");
      if (!pathEnv)
      {
        std::cout << target << ": not found\n";
        return true;
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
    return true;
  }
  else if (command == "history")
  {
    HIST_ENTRY **list = history_list();
    if (!list)
      return true;
    int total = history_length;
    if (filteredToken.size() == 1)
    {
      for (int i = 0; list[i] != nullptr; i++)
      {
        std::cout << "   " << i + 1 << " " << list[i]->line << "\n";
      }
    }
    else if (filteredToken[1] == "-r" && filteredToken.size() > 2)
    {
      read_history(filteredToken[2].c_str());
    }
    else if (filteredToken[1] == "-w" && filteredToken.size() > 2)
    {
      write_history(filteredToken[2].c_str());
      session_start_index = history_length;
    }
    else if (filteredToken[1] == "-a" && filteredToken.size() > 2)
    {
      int new_commands = history_length - session_start_index;
      if (new_commands > 0)
      {
        append_history(new_commands, filteredToken[2].c_str());
        session_start_index = history_length;
      }
    }
    else
    {
      int n = std::stoi(filteredToken[1]);
      int start = std::max(0, total - n);
      for (int i = start; list[i] != nullptr; i++)
      {
        std::cout << "   " << i + 1 << " " << list[i]->line << "\n";
      }
    }
    return true;
  }
  return false;
}