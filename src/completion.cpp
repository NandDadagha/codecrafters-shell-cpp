#include "completion.h"
#include "utils.h"

#include <vector>
#include <string>
#include <sstream>
#include <cstdlib>

#include <dirent.h>

#include <readline/readline.h>
#include <readline/history.h>

char *command_generator(const char *text, int state)
{
  static int index, len;
  std::vector<std::string> builtIn = {"echo", "exit", "history"};
  static std::vector<std::string> matches;
  if (state == 0)
  { // First time
    index = 0;
    len = strlen(text);
    matches.clear();

    for (int list_index = 0; list_index < builtIn.size(); list_index++)
    {
      const char *name = builtIn[list_index].c_str();
      if (strncmp(name, text, len) == 0)
      {
        matches.push_back(name);
      }
    }
    const char *pathEnv = getenv("PATH");
    std::stringstream ss(pathEnv);
    std::string dir;
    while (getline(ss, dir, ':'))
    {
      DIR *d = opendir(dir.c_str());
      if (!d)
        continue;

      struct dirent *entry;
      while (entry = readdir(d))
      {
        std::string name = entry->d_name;
        std::string full = dir + "/" + name;
        if (name.find(text) == 0 && isExecutable(full))
          matches.push_back(name);
      }
      closedir(d);
    }
  }
  if (index < matches.size())
  {
    std::string result = matches[index];
    index++;
    return strdup(result.c_str());
  }
  return nullptr;
}
char **completion_function(const char *text, int start, int end)
{
  if (start == 0)
  {
    return rl_completion_matches(text, command_generator);
  }
  // rl_attempted_completion_over = 1; This stops autocompletion done by readline()
  return nullptr;
}