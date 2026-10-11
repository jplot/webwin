#ifndef WEBWIN_PROCESS_H
#define WEBWIN_PROCESS_H

// What GetCommandLineA returns; call it from main, before the engine reads the command line.
void WebWin_SetCommandLine(const char *line);

#endif
