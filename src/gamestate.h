#ifndef GAMESTATE_H
#define GAMESTATE_H

#include <raylib.h>
#include "character.h"
#include "tespecial.h"
#include "buildtool.h"
#include "platform.h"
#include "pickup.h"

typedef struct GameState {
	Camera3D genericCamera;
	Camera3D playerCamera;
	int lenCharacters;
	Character characters[MAX_CHARACTERS];
	Model playerModel;
	TeSpecial playerTeSpecial;
	int lenBoxes;
	Box boxes[MAX_BOXES];
	int lenPlatforms;
	Platform platforms[MAX_PLATFORMS];
	Model unitCubeModel;
	Pickup pickups[MAX_PICKUPS];
	int lenPickups;
	Model teAmmoModel;
	Model donutModel;
	Shader basicLightingShader;
	float gravity;
} GameState;

#endif
