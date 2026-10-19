#include "console.h"
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "gamestate.h"

typedef void (*CommandFn)(Console *console, GameState *gameState, int lenArgs, char *args[]);

typedef struct Command {
	const char *name;
	CommandFn fn;
	const char *help;
} Command;

void cmdHelp(Console *console, GameState *gameState, int lenArgs, char *args[]);

void cmdClear(Console *console, GameState *gameState, int lenArgs, char *args[]) {
	(void)lenArgs;
	(void)args;
	(void)gameState;

	if (console != NULL) console->lenLog = 0;
}

void cmdEcho(Console *console, GameState *gameState, int lenArgs, char *args[]) {
	(void)console;
	(void)gameState;

	size_t size = 1;

	for (int i = 0; i < lenArgs; ++i) {
		size += strlen(args[i]);
		if (i > 0) ++size;
	}

	char *result = malloc(size);
	if (result == NULL)
		return;

	char *p = result;

	for (int i = 0; i < lenArgs; ++i) {
		if (i > 0) *p++ = ' ';

		size_t len = strlen(args[i]);
		memcpy(p, args[i], len);
		p += len;
	}

	*p = '\0';

	TraceLog(LOG_INFO, "%s", result);

	free(result);
}

void cmdGravity(Console *console, GameState *gameState, int lenArgs, char *args[]) {
	(void)console;
	(void)lenArgs;

	// Make sure we have an argument
	if (args[0] == NULL) {
		TraceLog(LOG_ERROR, "No argument for gravity provided.");
		TraceLog(LOG_ERROR, "Gravity usage: `gravity <floating point value>`");
		return;
	}

	char *endptr;
	float value = strtof(args[0], &endptr);

	if (endptr == args[0]) {
		TraceLog(LOG_ERROR, "%s could not be converted into a valid floating point number.", args[0]);
		return;
	}

	gameState->gravity = value;
}

static const Command commands[] = {
	{ "help",  cmdHelp,  "List available commands" },
	{ "clear", cmdClear, "Clear the console" },
	{ "echo",  cmdEcho,  "Print text" },
	{ "gravity",  cmdGravity,  "Set gravity value" },
};

void cmdHelp(Console *console, GameState *gameState, int lenArgs, char *args[]) {
	(void)console;
	(void)lenArgs;
	(void)gameState;

	if (args[0] == NULL) {
		TraceLog(LOG_INFO, "Welecome to the Josh's Raylib Sandbox console!");
		TraceLog(LOG_INFO, "If you're reading this text right now, that means you're probably pretty cool!");
		TraceLog(LOG_INFO, "The console is a command line interface of sorts. Here you can type in commands. Commands will usually trigger some sort of action if typed correctly.");
		TraceLog(LOG_INFO, "Use the \'`\' key to open and close this console.");
		TraceLog(LOG_INFO, "Here is a list of commands and what they do:");

		for (size_t i = 0; i < (sizeof(commands) / sizeof(commands[0])); i++) {
			TraceLog(LOG_INFO, "%s -- %s", commands[i].name, commands[i].help);
		}

		return;
	}

	bool validCommand = false;
	for (size_t i = 0; i < (sizeof(commands) / sizeof(commands[0])); i++) {
		if (strcmp(args[0], commands[i].name) == 0) {
			validCommand = true;
			TraceLog(LOG_INFO, "%s", commands[i].help);
			break;
		}
	}

	if (!validCommand) {
			TraceLog(LOG_ERROR, "help could not find a command called \"%s\".", args[0]);
	}
}

void AddConsoleLog(Console *console, int level, const char *text) {
	if (console == NULL || text == NULL) return;

	// Keep the newest entries when the fixed-size log is full.
	if (console->lenLog == CONSOLE_LOG_LENGTH) {
		memmove(&console->logs[0], &console->logs[1], sizeof(console->logs[0]) * (CONSOLE_LOG_LENGTH - 1));
		console->lenLog--;
	}

	ConsoleLogEntry *entry = &console->logs[console->lenLog++];
	entry->level = level;
	snprintf(entry->text, sizeof(entry->text), "%s", text);
}

void ExecuteCommand(Console *console, GameState *gameState) {
	char *input = console->command;

	// Trace out the input so it appears in the log
	TraceLog(LOG_INFO, "%s", input);

	// Parse the command
	char *command;
	int lenArgs = 0;
	command = strtok(input, " ");
	
	if (command == NULL) {
		return;
	}

	char *args[32];
	char *arg;
	while ((arg = strtok(NULL, " ")) != NULL) {
		if (lenArgs < 32) {
			args[lenArgs++] = arg;
		} else {
			TraceLog(LOG_ERROR, "Too many arguments.");
			return;
		}
	}

	// Run the command
	bool found = false;
	for (size_t i = 0; i < (sizeof(commands) / sizeof(commands[0])); i++) {
		if (strcmp(command, commands[i].name) == 0) {
			found = true;
			commands[i].fn(console, gameState, lenArgs, args);
			break;
		}
	}

	if (!found) {
		TraceLog(LOG_ERROR, "Command \"%s\" not found.", command);
	}
}

void UpdateConsole(Console *console, GameState *gameState) {
	// Get user input
	int key = GetCharPressed();

	while (key > 0) {
		if ((key >=32) && (key <= 125) && (key != 96) && (console->lenCommand < CONSOLE_COMMAND_LENGTH)) {
			console->command[console->lenCommand] = (char)key;
			console->command[console->lenCommand + 1] = '\0';
			console->lenCommand++;
		}

		key = GetCharPressed();
	}

	if (IsKeyPressed(KEY_BACKSPACE)) {
		console->lenCommand--;
		if (console->lenCommand < 0) console->lenCommand = 0;
		console->command[console->lenCommand] = '\0';
	}

	if (IsKeyDown(KEY_U) && IsKeyDown(KEY_LEFT_CONTROL)) {
		console->lenCommand = 0;
		console->command[console->lenCommand] = '\0';
	}

	if (IsKeyPressed(KEY_ENTER)) {
		ExecuteCommand(console, gameState);
		console->lenCommand = 0;
		console->command[console->lenCommand] = '\0';
	}

	// Update cursor timer
	console->cursorTimer++;
}

void DrawConsole(Console console, int screenWidth, int screenHeight) {
	// Draw log/feed area
	DrawRectangle(0, 0, screenWidth, screenHeight / 2, DARKGRAY);

	const int lineHeight = 20;
	const int maxLines = (screenHeight / 2 - 10) / lineHeight;
	int first = console.lenLog - maxLines;
	if (first < 0) first = 0;

	Color color = RAYWHITE;

	int y = 5;
	for (int i = first; i < console.lenLog; i++) {
		switch (console.logs[i].level) {
			case LOG_ERROR: color = RED; break;

			case LOG_FATAL: color = MAROON; break;

			case LOG_WARNING: color = YELLOW; break;

			case LOG_DEBUG: color = PURPLE; break;

			default: color = RAYWHITE;
		}

		DrawText(console.logs[i].text, 10, y, 16, color);
		y += lineHeight;
	}

	// Draw input area.
	DrawText(console.command, 10, y, 16, RAYWHITE);

	// Draw cursor
	if (console.lenCommand < CONSOLE_COMMAND_LENGTH) {
		if ((console.cursorTimer / 20) % 2 == 0) DrawText("_", 10 + MeasureText(console.command, 16), y, 16, RAYWHITE);
	}
}
