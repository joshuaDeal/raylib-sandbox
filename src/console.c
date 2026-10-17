#include "console.h"
#include <stddef.h>
#include <string.h>
#include <stdio.h>

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

void UpdateConsole(Console *console) {
	(void)console;
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
}
