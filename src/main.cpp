#include <iostream>
#include <string>
#include <cstdlib>    // getenv
#include <sstream>    // stringstream
#include <unistd.h>   // access
#include <sys/stat.h> // stat
using namespace std;

bool isExecutable(const string &path)
{
  struct stat sb;
  return stat(path.c_str(), &sb) == 0 &&
         S_ISREG(sb.st_mode) &&
         access(path.c_str(), X_OK) == 0;
}

int main()
{
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  while (1)
  {
    std::cout << "$ ";
    string command;
    getline(cin, command);

    // exit
    if (command == "exit")
      exit(0);

    // echo
    if (command.rfind("echo", 0) == 0)
    {
      string remaining = command.substr(4);
      if (!remaining.empty() && remaining[0] == ' ')
        remaining.erase(0, remaining.find_first_not_of(' '));
      cout << remaining << endl;
      continue;
    }

    // type
    if (command.rfind("type", 0) == 0)
    {
      string remaining = command.substr(4);
      if (!remaining.empty() && remaining[0] == ' ')
        remaining.erase(0, remaining.find_first_not_of(' '));

      if (remaining == "echo" || remaining == "exit" || remaining == "type")
      {
        cout << remaining << " is a shell builtin\n";
        continue;
      }
      const char *pathEnv = getenv("PATH");
      if (!pathEnv)
      {
        cout << remaining << ": not found\n";
        continue;
      }
      stringstream ss(pathEnv);
      string dir;
      bool flag = false;
      while (getline(ss, dir, ':'))
      {
        string fullPath = dir + "/" + remaining;

        // does file exists?
        if (access(fullPath.c_str(), F_OK) == 0)
        {
          // is it executable?
          if (isExecutable(fullPath))
          {
            cout << remaining << " is " << fullPath << "\n";
            flag = true;
            break;
          }
        }
      }
      if(!flag)
        cout << remaining << ": not found\n";
    continue;
    }
    cout << command << ": command not found" << endl;
  }
}
