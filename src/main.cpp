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
    if(command == "exit") exit(0);
    cout << command << ": command not found" << endl;
  }
}
