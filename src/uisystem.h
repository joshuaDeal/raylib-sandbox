#ifndef UISYSTEM_H
#define UISYSTEM_H

#include <raylib.h>

typedef struct MenuButton {
	Vector2 position;
	Vector2 size;
	int fontSize;
	Color buttonColor;
	Color borderColor;
	Color textColor;
	char *buttonText;
	bool hover;
	bool clicked;
	int boarderOffset;
	bool shadow;
	bool hoverSoundFlag;
} MenuButton;

void DrawMenuButton(MenuButton button);

void UpdateMenuButton(MenuButton *button, Sound hoverSound, Sound clickSound);

#endif
