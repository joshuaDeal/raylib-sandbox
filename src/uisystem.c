#include "uisystem.h"
#include <raymath.h>
#include "soundsystem.h"

void DrawMenuButton(MenuButton button) {
	// Shadow
	if (button.shadow) DrawRectangleV(Vector2Add(button.position, (Vector2){ 5, 5 }), button.size, Fade(BLACK, 0.5f));

	// Boarder
	DrawRectangleV(button.position, button.size, button.borderColor);

	// Button itself
	if (!button.hover) DrawRectangleV((Vector2){ button.position.x + (button.size.x - (button.size.x - button.boarderOffset)) / 2, button.position.y + (button.size.y - (button.size.y - button.boarderOffset)) / 2 }, Vector2Subtract(button.size, (Vector2){ button.boarderOffset, button.boarderOffset }), button.buttonColor);
	else if (button.hover) DrawRectangleV((Vector2){ button.position.x + (button.size.x - (button.size.x - button.boarderOffset)) / 2, button.position.y + (button.size.y - (button.size.y - button.boarderOffset)) / 2 }, Vector2Subtract(button.size, (Vector2){ button.boarderOffset, button.boarderOffset }), (Color){ fmin(255, button.buttonColor.r * 2), fmin(255, button.buttonColor.g * 2), fmin(255, button.buttonColor.b * 2), 255});

	// Text
	if (!button.hover) DrawText(button.buttonText, button.position.x + (button.boarderOffset / 2) + ((button.size.x - button.boarderOffset) - MeasureText(button.buttonText, button.fontSize)) / 2, button.position.y + (button.boarderOffset / 2) + ((button.size.y - button.boarderOffset) - button.fontSize) / 2, button.fontSize, button.textColor);
	else if (button.hover) DrawText(button.buttonText, button.position.x + (button.boarderOffset / 2) + ((button.size.x - button.boarderOffset) - MeasureText(button.buttonText, button.fontSize)) / 2, button.position.y + (button.boarderOffset / 2) + ((button.size.y - button.boarderOffset) - button.fontSize) / 2, button.fontSize, (Color){ fmin(255, button.textColor.r * 2), fmin(255, button.textColor.g * 2), fmin(255, button.textColor.b * 2), 255});
}

void UpdateMenuButton(MenuButton *button, Sound hoverSound, Sound clickSound) {
	Vector2 mousePosition = GetMousePosition();

	// Hover
	if (CheckCollisionPointRec(mousePosition, (Rectangle){ button->position.x, button->position.y, button->size.x, button->size.y })) {
		button->hover = true;

		if (!button->hoverSoundFlag) {
			PlayUISound(hoverSound, 0.5f);
			button->hoverSoundFlag = true;
		}
	}
	else {
		button->hover = false;
		button->hoverSoundFlag = false;
	}
	
	// Click
	if (button->hover && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
		button->clicked = true;
		PlayUISound(clickSound, 0.5f);
	}
}
