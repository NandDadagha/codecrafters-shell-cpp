#include <iostream>
#include <string>
#include <cstdlib>
using namespace std;

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
    if (command == "exit")
      exit(0);
    if (command.rfind("echo", 0) == 0)
    {
      string remaining = command.substr(4);
      if (!remaining.empty() && remaining[0] == ' ')
        remaining.erase(0, remaining.find_first_not_of(' '));
      cout << remaining << endl;
      continue;
    }
    cout << command << ": command not found" << endl;
  }
}
