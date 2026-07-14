#include "utils.h"

#include <unistd.h>
#include <sys/stat.h>

bool isExecutable(const std::string& path)
{
    struct stat sb;

    return stat(path.c_str(), &sb) == 0 &&
           S_ISREG(sb.st_mode) &&
           access(path.c_str(), X_OK) == 0;
}