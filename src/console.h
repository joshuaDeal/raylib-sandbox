#ifndef CONSOLE_H
#define CONSOLE_H

#define CONSOLE_LOG_LENGTH 100
#define CONSOLE_LOG_TEXT_SIZE 256
#define CONSOLE_COMMAND_LENGTH 256

#include <raylib.h>

typedef struct ConsoleLogEntry {
	int level;
	char text[CONSOLE_LOG_TEXT_SIZE];
} ConsoleLogEntry;

typedef struct Console {
	bool enabled;
	bool visible;
	ConsoleLogEntry logs[CONSOLE_LOG_LENGTH];
	int lenLog;
	char command[CONSOLE_COMMAND_LENGTH + 1];
	int lenCommand;
	int cursorTimer;
} Console;

void AddConsoleLog(Console *console, int level, const char *text);

void UpdateConsole(Console *console);

void DrawConsole(Console console, int screenWidth, int screenHeight);

#endif
