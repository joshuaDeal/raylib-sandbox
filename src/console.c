#include "console.h"
#include "gamestate.h"
#include "platform.h"
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <raymath.h>

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

	// Make sure we have an argument
	if (lenArgs < 1) {
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

void cmdSpawn(Console *console, GameState *gameState, int lenArgs, char *args[]) {
	(void)console;

	// Make sure we have an argument
	if (lenArgs < 1) {
		TraceLog(LOG_ERROR, "No argument for spawn provided.");
		TraceLog(LOG_ERROR, "Spawn usage: `spawn <type> <sub-type> <option> <x position> <y position> <z position>`");
		return;
	}

	bool validObject = false;

	// Pickups
	if (strcmp(args[0], "pickup") == 0 && gameState->lenPickups < MAX_PICKUPS) {
		validObject = true;

		// Make sure we have a sub-type
		if (lenArgs < 2) {
			TraceLog(LOG_ERROR, "No sub-type argument for spawn pickup provided.");
			TraceLog(LOG_ERROR, "Spawn pickup usage: `spawn pickup <sub-type> <can respawn (true or false)> <x position> <y position> <z position>`");
			return;
		}

		// Evaluate position
		Vector3 position = Vector3Add(gameState->playerCamera.position, (Vector3){ 1.0, 0.0, 0.0 });

		if (lenArgs > 3) {
			// Check that all positional values are provided
			if (lenArgs < 6) {
				TraceLog(LOG_ERROR, "Missing an argument.");
				TraceLog(LOG_ERROR, "Spawn pickup usage: `spawn pickup <sub-type> <can respawn (true or false)> <x position> <y position> <z position>`");
				return;
			}

			float values[3];
			for (int i = 3, j = 0; i < 6; i++, j++) {
				// Try and convert to float
				char *endptr;
				values[j] = strtof(args[i], &endptr);
		
				if (endptr == args[i]) {
					TraceLog(LOG_ERROR, "%s could not be converted into a valid floating point number.", args[i]);
					return;
				}
			}

			position = (Vector3){ values[0], values[1], values[2] };
		}

		// Toggle if pickup can respawn.
		bool canRespawn = true;
		if (lenArgs > 2) {
			if (strcmp(args[2], "false") == 0) {
				canRespawn = false;
			} else if (strcmp(args[2], "true") != 0) {
				TraceLog(LOG_ERROR, "%s should be 'true' or 'false'.", args[2]);
				return;
			}
		}

		if (strcmp(args[1], "teAmmo") == 0) {
			gameState->pickups[gameState->lenPickups] = (Pickup){ .type = TESPECIAL_AMMO, .canRespawn = canRespawn, .respawnTime = 40.0f, .respawnTimer = 0.0f, .enabled = true, .position = position, .size = (Vector3){ 0.5f, 0.5f, 0.5f }, .model = gameState->teAmmoModel, .spin = 0.0f, .spinSpeed = 1.0f, .targetY = position.y, .bounceUp = true, .bounceTime = 0.0f, .bounceDuration = 1.0f };
			gameState->pickups[gameState->lenPickups].light = CreateLight(LIGHT_POINT, gameState->pickups[gameState->lenPickups].position, Vector3Zero(), (Color){ 255, 214, 100, 255 }, 0.3f, gameState->basicLightingShader);
			gameState->lenPickups++;
		}
		else if (strcmp(args[1], "donut") == 0) {
			gameState->pickups[gameState->lenPickups] = (Pickup){ .type = DONUT, .canRespawn = canRespawn, .respawnTime = 40.0f, .respawnTimer = 0.0f, .enabled = true, .position = position, .size = (Vector3){ 0.5f, 0.5f, 0.5f }, .model = gameState->donutModel, .spin = 0.0f, .spinSpeed = 1.0f, .targetY = position.y, .bounceUp = true, .bounceTime = 0.0f, .bounceDuration = 1.0f };
			gameState->pickups[gameState->lenPickups].light = CreateLight(LIGHT_POINT, gameState->pickups[gameState->lenPickups].position, Vector3Zero(), (Color){ 255, 214, 100, 255 }, 0.3f, gameState->basicLightingShader);
			gameState->lenPickups++;
		}
		else {
			TraceLog(LOG_ERROR, "pickup named '%s' not found.", args[1]);
			return;
		}
	}

	// NPCs 
	if (strcmp(args[0], "npc") == 0 && gameState->lenCharacters < MAX_CHARACTERS) {
		validObject = true;

		// Evaluate position
		Vector3 position = Vector3Add(gameState->playerCamera.position, (Vector3){ 1.0, 0.0, 0.0 });

		if (lenArgs > 1) {
			// Check that all positional values are provided
			if (lenArgs < 4) {
				TraceLog(LOG_ERROR, "Missing positional argument.");
				return;
			}

			float values[3];
			for (int i = 1, j = 0; i < 4; i++, j++) {
				// Try and convert to float
				char *endptr;
				values[j] = strtof(args[i], &endptr);
		
				if (endptr == args[i]) {
					TraceLog(LOG_ERROR, "%s could not be converted into a valid floating point number.", args[i]);
					return;
				}
			}

			position = (Vector3){ values[0], values[1], values[2] };
		}

		gameState->characters[gameState->lenCharacters] = (Character){ 0 };
		gameState->characters[gameState->lenCharacters].size = (Vector3){ 0.90f, 1.90f, 0.90f };
		gameState->characters[gameState->lenCharacters].position = position;
		gameState->characters[gameState->lenCharacters].isHuman = false;
		gameState->characters[gameState->lenCharacters].isPlayer = false;
		gameState->characters[gameState->lenCharacters].speed = PLAYER_SPEED;
		gameState->characters[gameState->lenCharacters].jumpForce = PLAYER_JUMP_FORCE;
		gameState->characters[gameState->lenCharacters].yaw = 3.14f;
		gameState->characters[gameState->lenCharacters].pitch = 0.0f;
		gameState->characters[gameState->lenCharacters].model = gameState->playerModel;
		gameState->characters[gameState->lenCharacters].footstepTimer = 0.0f;
		gameState->characters[gameState->lenCharacters].jumpBoost = false;
		gameState->characters[gameState->lenCharacters].landFlag = false;
		for (int i = 0; i < NUM_INVENTORY_ITEMS; i++) gameState->characters[gameState->lenCharacters].inventory[i] = false;
		gameState->characters[gameState->lenCharacters].inventoryIndex = 0;
		gameState->characters[gameState->lenCharacters].health = 100.0f;
		gameState->characters[gameState->lenCharacters].color = BLUE;
		gameState->lenCharacters++;
	}

	if (strcmp(args[0], "platform") == 0 && gameState->lenPlatforms < MAX_PLATFORMS) {
		validObject = true;

		// Evaluate color.
		// TODO: I should probably be using rgba.
		Color color = GREEN;
		if (lenArgs > 1) {
			if (strcmp(args[1], "LIGHTGRAY") == 0) color = LIGHTGRAY;
			else if (strcmp(args[1], "GRAY") == 0) color = GRAY;
			else if (strcmp(args[1], "DARKGRAY") == 0) color = DARKGRAY;
			else if (strcmp(args[1], "YELLOW") == 0) color = YELLOW;
			else if (strcmp(args[1], "GOLD") == 0) color = GOLD;
			else if (strcmp(args[1], "ORANGE") == 0) color = ORANGE;
			else if (strcmp(args[1], "PINK") == 0) color = PINK;
			else if (strcmp(args[1], "RED") == 0) color = RED;
			else if (strcmp(args[1], "MAROON") == 0) color = MAROON;
			else if (strcmp(args[1], "GREEN") == 0) color = GREEN;
			else if (strcmp(args[1], "LIME") == 0) color = LIME;
			else if (strcmp(args[1], "DARKGREEN") == 0) color = DARKGREEN;
			else if (strcmp(args[1], "SKYBLUE") == 0) color = SKYBLUE;
			else if (strcmp(args[1], "BLUE") == 0) color = BLUE;
			else if (strcmp(args[1], "DARKBLUE") == 0) color = DARKBLUE;
			else if (strcmp(args[1], "PURPLE") == 0) color = PURPLE;
			else if (strcmp(args[1], "VIOLET") == 0) color = VIOLET;
			else if (strcmp(args[1], "DARKPURPLE") == 0) color = DARKPURPLE;
			else if (strcmp(args[1], "BEIGE") == 0) color = BEIGE;
			else if (strcmp(args[1], "BROWN") == 0) color = BROWN;
			else if (strcmp(args[1], "DARKBROWN") == 0) color = DARKBROWN;
			else if (strcmp(args[1], "WHITE") == 0) color = WHITE;
			else if (strcmp(args[1], "BLACK") == 0) color = BLACK;
			else if (strcmp(args[1], "MAGENTA") == 0) color = MAGENTA;
			else if (strcmp(args[1], "RAYWHITE") == 0) color = RAYWHITE;
			else {
				TraceLog(LOG_ERROR, "%s is not a valid color.", args[1]);
				return;
			}
		}

		// Evaluate solid
		bool solid = true;
		if (lenArgs > 2) {
			if (strcmp(args[2], "false") == 0) {
				solid = false;
			} else if (strcmp(args[2], "true") != 0) {
				TraceLog(LOG_ERROR, "%s should be 'true' or 'false'.", args[2]);
				return;
			}
		}

		// Evaluate size 
		Vector3 size = (Vector3){ 1.0f, 0.5f, 1.0f };
		if (lenArgs > 4) {
			// Check that all positional values are provided
			if (lenArgs < 5) {
				TraceLog(LOG_ERROR, "Missing size argument.");
				TraceLog(LOG_ERROR, "Spawn platform usage: `spawn platform <color> <is solid (true or false)> <x size> <y size> <z size> <x position> <y position> <z position>`");
				return;
			}

			float values[3];
			for (int i = 3, j = 0; i < 6; i++, j++) {
				// Try and convert to float
				char *endptr;
				values[j] = strtof(args[i], &endptr);
		
				if (endptr == args[i]) {
					TraceLog(LOG_ERROR, "%s could not be converted into a valid floating point number.", args[i]);
					return;
				}
			}

			size = (Vector3){ values[0], values[1], values[2] };
		}

		// Evaluate position
		Vector3 position = Vector3Add(gameState->playerCamera.position, (Vector3){ 1.0, 0.0, 0.0 });
		if (lenArgs > 6) {
			// Check that all positional values are provided
			if (lenArgs < 8) {
				TraceLog(LOG_ERROR, "Missing positional argument.");
				TraceLog(LOG_ERROR, "Spawn platform usage: `spawn platform <color> <is solid (true or false)> <x size> <y size> <z size> <x position> <y position> <z position>`");
				return;
			}

			float values[3];
			for (int i = 6, j = 0; i < 8; i++, j++) {
				// Try and convert to float
				char *endptr;
				values[j] = strtof(args[i], &endptr);
		
				if (endptr == args[i]) {
					TraceLog(LOG_ERROR, "%s could not be converted into a valid floating point number.", args[i]);
					return;
				}
			}

			position = (Vector3){ values[0], values[1], values[2] };
		}

		// Spawn platform
		gameState->platforms[gameState->lenPlatforms] = (Platform){ .position = position, .size = size, .color = color, .solid = solid };
		gameState->platforms[gameState->lenPlatforms].model = CreatePlatformModel(gameState->platforms[gameState->lenPlatforms].size, gameState->basicLightingShader);
		gameState->lenPlatforms++;
	}

	if (!validObject) {
		TraceLog(LOG_ERROR, "%s not spawnable.", args[0]);
		return;
	}
}

void cmdDelete(Console *console, GameState *gameState, int lenArgs, char *args[]) {
	(void)console;

	// Make sure we have two arguments
	if (lenArgs < 2) {
		TraceLog(LOG_ERROR, "Inncorect number of arguments for delete.");
		TraceLog(LOG_ERROR, "Delete usage: `delete <type> <id number>`");
		return;
	}

	bool validObject = false;

	// Pickups
	if (strcmp(args[0], "pickup") == 0 && gameState->lenPickups <= MAX_PICKUPS) {
		validObject = true;

		int position = atoi(args[1]);

		if (gameState->lenPickups > position && position >= 0) {
			// Remove pickup light
			DeleteLight(gameState->basicLightingShader, &gameState->pickups[position].light);

			// Remove pickup
			for (int i = position; i < gameState->lenPickups - 1; i++) {
				gameState->pickups[i] = gameState->pickups[i + 1];
			}
			gameState->lenPickups--;
		}
		else {
			TraceLog(LOG_ERROR, "Pickup %d not in range.", position);
			return;
		}
	}
	
	// NPCs 
	if (strcmp(args[0], "npc") == 0 && gameState->lenCharacters <= MAX_CHARACTERS) {
		validObject = true;

		int position = atoi(args[1]);

		if (gameState->lenCharacters > position && position >= 0 && !gameState->characters[position].isPlayer) {
			// Remove npc 
			for (int i = position; i < gameState->lenCharacters - 1; i++) {
				gameState->characters[i] = gameState->characters[i + 1];
			}
			gameState->lenCharacters--;
		}
		else {
			TraceLog(LOG_ERROR, "NPC %d not in range.", position);
			return;
		}
	}

	// Platforms
	if (strcmp(args[0], "platform") == 0 && gameState->lenPlatforms <= MAX_PLATFORMS) {
		validObject = true;

		int position = atoi(args[1]);

		if (gameState->lenPlatforms > position && position >= 0) {
			// Unload platform model
			UnloadModel(gameState->platforms[position].model);

			// Remove platform 
			for (int i = position; i < gameState->lenPlatforms - 1; i++) {
				gameState->platforms[i] = gameState->platforms[i + 1];
			}
			gameState->lenPlatforms--;
		}
		else {
			TraceLog(LOG_ERROR, "Platform %d not in range.", position);
			return;
		}
	}

	if (!validObject) {
		TraceLog(LOG_ERROR, "%s not deletable.", args[0]);
		return;
	}
}

void cmdNoclip(Console *console, GameState *gameState, int lenArgs, char *args[]) {
	(void)console;
	(void)args;

	// Make sure we don't have any arguments
	if (lenArgs < 0) {
		TraceLog(LOG_ERROR, "Noclip doesn't need any arguments.");
		return;
	}

	// Find the player
	for (int i = 0; i < gameState->lenCharacters; i++) {
		if (gameState->characters[i].isPlayer) {
			// Toggle noclip
			gameState->characters[i].noclip = !gameState->characters[i].noclip;
		}
	}
}

static const Command commands[] = {
	{ "help",  cmdHelp,  "List available commands" },
	{ "clear", cmdClear, "Clear the console" },
	{ "echo",  cmdEcho,  "Print text" },
	{ "gravity",  cmdGravity,  "Set gravity value" },
	{ "spawn",  cmdSpawn,  "Spawn something" },
	{ "delete",  cmdDelete,  "Delete something" },
	{ "noclip",  cmdNoclip,  "Toggle noclip mode" },
};

void cmdHelp(Console *console, GameState *gameState, int lenArgs, char *args[]) {
	(void)console;
	(void)gameState;

	if (lenArgs < 1) {
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
