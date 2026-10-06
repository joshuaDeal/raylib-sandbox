#ifndef CHARACTER_H
#define CHARACTER_H

#define PLAYER_SPEED  5.0f
#define PLAYER_JUMP_FORCE 5.0f
#define NUM_INVENTORY_ITEMS 3
#define COLLISION_EPSILON 0.0001f

#include "soundsystem.h"

typedef struct Box Box;
typedef struct TeSpecial TeSpecial;
typedef struct Pickup Pickup;

typedef enum InventoryItems {
	INVENTORY_NOTHING = 0,
	INVENTORY_BUILD_TOOL = 1,
	INVENTORY_TESPECIAL = 2
} InventoryItems;

typedef struct Character {
	Vector3 position;
	Vector3 size;
	Vector3 delta;
	Vector3 velocity;
	float speed;
	float jumpForce;
	bool isHuman;
	bool onGround;
	float yaw; // Rotation around y
	float pitch; // Rotation around x
	Model model;
	float footstepTimer;
	bool jumpBoost;
	bool landFlag;
	bool inventory[NUM_INVENTORY_ITEMS];
	int inventoryIndex;
} Character;

void UpdateCharacterInventory(Character *character, bool *showInventory, float *showInventoryTimer, Box *boxes, int *lenBoxes, Ray *boxClickRay, RayCollision boxClickCollision, int *boxSelectedColorY, int *boxSelectedColorX, bool *showColorPicker, float *colorPickerTimer, Color colors[][8], int colorsPerRow, int colorsPerCol, Camera characterCamera, Model boxModel, Sound fxHitBox, Sound fxBreakBox, Sound fxPlaceBox, Sound fxChangeColor, TeSpecial *characterTeSpecial, SoundPool *teShotPool, SoundPool *reloadPool, SoundPool *triggerPullPool, bool *showTeSpecialHud);

void UpdateCharacter(Character *character, Camera3D *camera, float mouseSensitivity, float gravity, Box *objects, int lenObjects, Pickup *pickups, int lenPickups, TeSpecial *teSpecial, Sound walkSound, Sound jumpSound, Sound landSound, Shader lightShader);

#endif
