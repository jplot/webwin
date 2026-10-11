#include <windows.h>
#include <webwin/process.h>

#include <cstring>

static char s_commandLine[2048];

extern "C" char *GetCommandLineA() { return s_commandLine; }

void WebWin_SetCommandLine(const char *line)
{
    strncpy(s_commandLine, line, sizeof(s_commandLine) - 1);
}
