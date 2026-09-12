/*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*
My raylib sandbox :)

So far, I've been compiling with a command like:
`gcc -o ./sandbox.bin ./src/sandbox.c -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -lcjson`

There is also a makefile.
*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*/

#include <raylib.h>
#include <raymath.h>
#include <float.h>
#include <cjson/cJSON.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PLAYER_SPEED  5.0f
#define PLAYER_JUMP_FORCE 5.0f
#define MAX_BOXES 1000
#define BOX_SIZE 1.0f
#define MAX_BOX_PLACE_DISTANCE 3.0f
#define MAX_BOX_DELETE_DISTANCE 3.0f
#define COLLISION_EPSILON 0.0001f
#define MAX_DONUTS 100
#define MAX_SOUND_POOL_SIZE 4

typedef enum GameScreen { MENU, GAMEPLAY } GameScreen;

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
} Character;

typedef struct Box {
	Vector3 position;
	Vector3 size;
	Model model;
	Color color;
	float health;
} Box;

typedef struct Pickup {
	Vector3 position;
	Vector3 size;
	Model model;
	float spin;
	float spinSpeed;
	float targetY;
	bool bounceUp;
	float bounceTime;
	float bounceDuration;
	Sound idleSoundPool[MAX_SOUND_POOL_SIZE];
	int lenIdleSoundPool;
} Pickup;

// Save game to save file
bool SaveGameData(const char *filePath, const Character *player, const Box *boxes, int lenBoxes) {
	// Create json root
	cJSON *root = cJSON_CreateObject();

	// If we couldn't create the root, return false
	if (root == NULL) return false;

	// Meta-data about the save file
	cJSON_AddNumberToObject(root, "version", 1);
	cJSON_AddNumberToObject(root, "timestamp", time(NULL));

	// Player
	cJSON *playerJson = cJSON_AddObjectToObject(root, "player");

	cJSON *positionJson = cJSON_AddObjectToObject(playerJson, "position");
	cJSON_AddNumberToObject(positionJson, "x", player->position.x);
	cJSON_AddNumberToObject(positionJson, "y", player->position.y);
	cJSON_AddNumberToObject(positionJson, "z", player->position.z);

	cJSON *velocityJson = cJSON_AddObjectToObject(playerJson, "velocity");
	cJSON_AddNumberToObject(velocityJson, "x", player->velocity.x);
	cJSON_AddNumberToObject(velocityJson, "y", player->velocity.y);
	cJSON_AddNumberToObject(velocityJson, "z", player->velocity.z);

	cJSON_AddNumberToObject(playerJson, "yaw", player->yaw);
	cJSON_AddNumberToObject(playerJson, "pitch", player->pitch);
	cJSON_AddBoolToObject(playerJson, "isHuman", player->isHuman);

	// Boxes
	cJSON *boxesJson = cJSON_AddArrayToObject(root, "boxes");

	for (int i = 0; i < lenBoxes; i++) {
		cJSON *boxJson = cJSON_CreateObject();

		cJSON *boxPositionJson = cJSON_AddObjectToObject(boxJson, "position");

		cJSON_AddNumberToObject(boxPositionJson, "x", boxes[i].position.x);
		cJSON_AddNumberToObject(boxPositionJson, "y", boxes[i].position.y);
		cJSON_AddNumberToObject(boxPositionJson, "z", boxes[i].position.z);

		cJSON *colorJson = cJSON_AddObjectToObject(boxJson, "color");

		cJSON_AddNumberToObject(colorJson, "r", boxes[i].color.r);
		cJSON_AddNumberToObject(colorJson, "g", boxes[i].color.g);
		cJSON_AddNumberToObject(colorJson, "b", boxes[i].color.b);
		cJSON_AddNumberToObject(colorJson, "a", boxes[i].color.a);

		cJSON_AddNumberToObject(boxJson, "health", boxes[i].health);

		cJSON_AddItemToArray(boxesJson, boxJson);
	}

	// Convert the JSON tree into a formatted string
	char *jsonString = cJSON_Print(root);

	// Return false if we don't have a jsonString
	if (jsonString == NULL) {
		cJSON_Delete(root);
		return false;
	}

	// Opening with "w" overwrites the file if it already exists.
	FILE *file = fopen(filePath, "w");

	if (file == NULL) {
		cJSON_free(jsonString);
		cJSON_Delete(root);
		return false;
	}

	if (fputs(jsonString, file) == EOF) {
		fclose(file);
		cJSON_free(jsonString);
		cJSON_Delete(root);
		return false;
	}

	fclose(file);

	cJSON_free(jsonString);
	cJSON_Delete(root);

	return true;
}

// Load game data from save file.
bool LoadGameData(const char *filePath, Character *player, Box *boxes, int *lenBoxes, Model boxModel) {
	Box tmpBoxes[MAX_BOXES];

	// Read the entire file into memory.
	FILE *file = fopen(filePath, "r");

	if (file == NULL) return false;

	// Put the file position indicator at end of file.
	fseek(file, 0, SEEK_END);
	// ftell() gives us the current file position indicator. Since it's at the end of the file, this gives us the file size.
	long fileSize = ftell(file);
	// Move file position indicator back to the beginning of the file.
	rewind(file);

	// If ftell had failed, fileSize would be negative.
	if (fileSize < 0) {
		fclose(file);
		return false;
	}

	// Allocate enough memory for every byte in the file, plus one byte for the terminating null character
	char *jsonString = malloc(fileSize + 1);

	if (jsonString == NULL) {
		fclose(file);
		return false;
	}

	// Populates jsonString with data from file.
	size_t bytesRead = fread(jsonString, 1, fileSize, file);

	fclose(file);

	// bytesRead being the wrong size would mean that something went wrong with reading the file.
	if (bytesRead != (size_t)fileSize) {
		free(jsonString);
		return false;
	}

	jsonString[fileSize] = '\0';

	// Parse the JSON string.
	cJSON *root = cJSON_Parse(jsonString);

	free(jsonString);

	if (root == NULL) return false;

	// Version
	cJSON *versionJson = cJSON_GetObjectItemCaseSensitive(root, "version");

	if (!cJSON_IsNumber(versionJson) || versionJson->valueint != 1) {
		cJSON_Delete(root);
		return false;
	}

	// Player
	cJSON *playerJson = cJSON_GetObjectItemCaseSensitive(root, "player");

	if (!cJSON_IsObject(playerJson)) {
		cJSON_Delete(root);
		return false;
	}

	cJSON *positionJson = cJSON_GetObjectItemCaseSensitive(playerJson, "position");
	cJSON *velocityJson = cJSON_GetObjectItemCaseSensitive(playerJson, "velocity");
	cJSON *yawJson = cJSON_GetObjectItemCaseSensitive(playerJson, "yaw");
	cJSON *pitchJson = cJSON_GetObjectItemCaseSensitive(playerJson, "pitch");
	cJSON *isHumanJson = cJSON_GetObjectItemCaseSensitive(playerJson, "isHuman");

	if (!cJSON_IsObject(positionJson) || !cJSON_IsObject(velocityJson) || !cJSON_IsNumber(yawJson) || !cJSON_IsNumber(pitchJson) || !cJSON_IsBool(isHumanJson)) {
		cJSON_Delete(root);
		return false;
	}

	cJSON *playerPositionX = cJSON_GetObjectItemCaseSensitive(positionJson, "x");
	cJSON *playerPositionY = cJSON_GetObjectItemCaseSensitive(positionJson, "y");
	cJSON *playerPositionZ = cJSON_GetObjectItemCaseSensitive(positionJson, "z");
	cJSON *velocityX = cJSON_GetObjectItemCaseSensitive(velocityJson, "x");
	cJSON *velocityY = cJSON_GetObjectItemCaseSensitive(velocityJson, "y");
	cJSON *velocityZ = cJSON_GetObjectItemCaseSensitive(velocityJson, "z");

	if (!cJSON_IsNumber(playerPositionX) || !cJSON_IsNumber(playerPositionY) || !cJSON_IsNumber(playerPositionZ) || !cJSON_IsNumber(velocityX) || !cJSON_IsNumber(velocityY) || !cJSON_IsNumber(velocityZ)) {
		cJSON_Delete(root);
		return false;
	}

	// Boxes
	cJSON *boxesJson = cJSON_GetObjectItemCaseSensitive(root, "boxes");

	if (!cJSON_IsArray(boxesJson)) {
		cJSON_Delete(root);
		return false;
	}

	int boxCount = cJSON_GetArraySize(boxesJson);

	if (boxCount > MAX_BOXES) {
		cJSON_Delete(root);
		return false;
	}

	for (int i = 0; i < boxCount; i++) {
		cJSON *boxJson = cJSON_GetArrayItem(boxesJson, i);

		if (!cJSON_IsObject(boxJson)) {
			cJSON_Delete(root);
			return false;
		}

		cJSON *positionJson = cJSON_GetObjectItemCaseSensitive(boxJson, "position");
		cJSON *colorJson = cJSON_GetObjectItemCaseSensitive(boxJson, "color");
		cJSON *healthJson = cJSON_GetObjectItemCaseSensitive(boxJson, "health");

		if (!cJSON_IsObject(positionJson) || !cJSON_IsObject(colorJson) || !cJSON_IsNumber(healthJson)) {
			cJSON_Delete(root);
			return false;
		}

		cJSON *positionX = cJSON_GetObjectItemCaseSensitive(positionJson, "x");
		cJSON *positionY = cJSON_GetObjectItemCaseSensitive(positionJson, "y");
		cJSON *positionZ = cJSON_GetObjectItemCaseSensitive(positionJson, "z");

		cJSON *colorR = cJSON_GetObjectItemCaseSensitive(colorJson, "r");
		cJSON *colorG = cJSON_GetObjectItemCaseSensitive(colorJson, "g");
		cJSON *colorB = cJSON_GetObjectItemCaseSensitive(colorJson, "b");
		cJSON *colorA = cJSON_GetObjectItemCaseSensitive(colorJson, "a");

		if (!cJSON_IsNumber(positionX) || !cJSON_IsNumber(positionY) || !cJSON_IsNumber(positionZ) || !cJSON_IsNumber(colorR) || !cJSON_IsNumber(colorG) || !cJSON_IsNumber(colorB) || !cJSON_IsNumber(colorA)) {
			cJSON_Delete(root);
			return false;
		}

		tmpBoxes[i].position.x = (float)positionX->valuedouble;
		tmpBoxes[i].position.y = (float)positionY->valuedouble;
		tmpBoxes[i].position.z = (float)positionZ->valuedouble;

		tmpBoxes[i].color.r = (unsigned char)colorR->valueint;
		tmpBoxes[i].color.g = (unsigned char)colorG->valueint;
		tmpBoxes[i].color.b = (unsigned char)colorB->valueint;
		tmpBoxes[i].color.a = (unsigned char)colorA->valueint;

		tmpBoxes[i].health = (float)healthJson->valuedouble;
	}

	*lenBoxes = boxCount;

	player->position.x = (float)playerPositionX->valuedouble;
	player->position.y = (float)playerPositionY->valuedouble;
	player->position.z = (float)playerPositionZ->valuedouble;

	player->velocity.x = (float)velocityX->valuedouble;
	player->velocity.y = (float)velocityY->valuedouble;
	player->velocity.z = (float)velocityZ->valuedouble;

	player->yaw = (float)yawJson->valuedouble;
	player->pitch = (float)pitchJson->valuedouble;
	player->isHuman = cJSON_IsTrue(isHumanJson);

	// Copy tmpBoxes to boxes.
	for (int i = 0; i < boxCount; i++) {
		boxes[i] = tmpBoxes[i];
		boxes[i].model = boxModel;
		boxes[i].size = (Vector3){ BOX_SIZE, BOX_SIZE, BOX_SIZE };
	}

	cJSON_Delete(root);

	return true;
}

// Draw menu buttons
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

void PlayUISound(Sound sound, float volume) {
	SetSoundVolume(sound, volume);
	SetSoundPan(sound, 0.0f);
	PlaySound(sound);
}

// Update menu buttons
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

void DrawFloor(int slicesX, int slicesZ, float spacing, float xMin, float zMin) {
	// Draw lines parallel to X (vary Z)
	for (int zi = 0; zi <= slicesZ; zi++) {
		float z = zMin + (float)zi * spacing;
		DrawLine3D((Vector3){xMin, 0.0f, z}, (Vector3){xMin + slicesX * spacing, 0.0f, z}, WHITE);
	}

	// Draw lines parallel to Z (vary X)
	for (int xi = 0; xi <= slicesX; xi++) {
		float x = xMin + (float)xi * spacing;
		DrawLine3D((Vector3){x, 0.0f, zMin}, (Vector3){x, 0.0f, zMin + slicesZ * spacing}, WHITE);
	}
}

void UpdatePositionalSound(Sound sound, Camera listener, Vector3 position, float maxDistance) {
	// Calculate direction and distance
	Vector3 direction = Vector3Subtract(position, listener.position);
	float distance = Vector3Length(direction);

	// Calculate attenuation
	float attenuation = 1.0f/(1.0f + (distance/maxDistance));
	attenuation = Clamp(attenuation, 0.0f, 1.0f);

	// Calculate normalized vectors
	Vector3 normalizedDirection = Vector3Normalize(direction);
	Vector3 forward = Vector3Normalize(Vector3Subtract(listener.target, listener.position));
	Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, listener.up));

	// Reduce volume for sounds behind the listener
	float dotProduct = Vector3DotProduct(forward, normalizedDirection);
	if (dotProduct < 0.0f) attenuation *= (1.0f + dotProduct*0.5f);

	// Set stereo panning
	float pan = 0.5f + 0.5f*Vector3DotProduct(normalizedDirection, right);

	// Apply changes to sound
	SetSoundVolume(sound, attenuation);
	SetSoundPan(sound, pan);
}

void PlayPositionalSound(Sound sound, Camera listener, Vector3 position, float maxDistance) {
	UpdatePositionalSound(sound, listener, position, maxDistance);
	PlaySound(sound);
}

void UpdateCharacter(Character *character, Camera3D *camera, float mouseSensitivity, float gravity, Box objects[], int lenObjects, Sound walkSound, Sound jumpSound, Sound landSound) {
	float delta = GetFrameTime();

	character->delta.x = 0.0f;
	character->delta.z = 0.0f;

	// Movement
	// ------------------------------------------
	// Mouse look
	if (character->isHuman) {
		Vector2 mouseDelta = GetMouseDelta();

		character->yaw += mouseDelta.x * mouseSensitivity;
		character->pitch -= mouseDelta.y * mouseSensitivity;

		// Clamp pitch
		float limit = 1.55f;
		if (character->pitch > limit) character->pitch = limit;
		if (character->pitch < -limit) character->pitch = -limit;
	}

	// Walking
	// Account for yaw
	Vector3 forward = { cosf(character->yaw), 0.0f, sinf(character->yaw) };
	Vector3 right = { -sinf(character->yaw), 0.0f, cosf(character->yaw) };

	if (character->isHuman) {
		if (IsKeyDown(KEY_W)) character->delta = Vector3Add(character->delta, forward);
		if (IsKeyDown(KEY_S)) character->delta = Vector3Subtract(character->delta, forward);
		if (IsKeyDown(KEY_A)) character->delta = Vector3Subtract(character->delta, right);
		if (IsKeyDown(KEY_D)) character->delta = Vector3Add(character->delta, right);
	}

	float distance = sqrtf(character->delta.x * character->delta.x + character->delta.z * character->delta.z);

	if (distance > 0.0f) {
		character->delta.x /= distance;
		character->delta.y /= distance;
	}

	character->velocity.x = character->delta.x * character->speed;
	character->velocity.z = character->delta.z * character->speed;

	// Jump stuff
	// Boost
	if ((!character->onGround) && character->jumpBoost) {
		character->speed *= 0.75;
		character->jumpBoost = false;
	} else if (character->onGround){
		character->speed = PLAYER_SPEED;
	}

	// Gravity
	if (!character->onGround) {
		character->velocity.y -= gravity * delta;
	}

	if (character->isHuman) {
		// Jump
		if (IsKeyPressed(KEY_SPACE) && character->onGround) {
			character->velocity.y = character->jumpForce;
			character->onGround = false;
			character->jumpBoost = true;
			character->landFlag = true;
			PlayPositionalSound(jumpSound, *camera, (Vector3){ character->position.x, character->position.y - character->size.y, character->position.z }, 7.0f);
		}
	}

	// Landing sound
	if (character->onGround && character->landFlag) {
		PlayPositionalSound(landSound, *camera, (Vector3){ character->position.x, character->position.y - character->size.y, character->position.z }, 7.0f);
		character->landFlag = false;
	}

	// Move x
	character->position.x += character->velocity.x * delta;

	// X collision
	for (int o = 0; o < lenObjects; o++) {
		if (character->position.y - character->size.y / 2.0f < objects[o].position.y + objects[o].size.y / 2.0f - COLLISION_EPSILON && character->position.y + character->size.y / 2.0f > objects[o].position.y - objects[o].size.y / 2.0f + COLLISION_EPSILON) {
			if (character->position.z - character->size.z / 2.0f < objects[o].position.z + objects[o].size.z / 2.0f && character->position.z + character->size.z / 2.0f > objects[o].position.z - objects[o].size.z / 2.0f) {
				if (character->position.x + character->size.x / 2.0f > objects[o].position.x - objects[o].size.x / 2.0f && character->position.x - character->size.x / 2.0f < objects[o].position.x + objects[o].size.x / 2.0f) {
					if (character->velocity.x > 0.0f) {
						character->position.x = objects[o].position.x - objects[o].size.x / 2.0f - character->size.x / 2.0f;
		
						character->velocity.x = 0.0f;
					}
					else if (character->velocity.x < 0.0f) {
						character->position.x = objects[o].position.x + objects[o].size.x / 2.0f + character->size.x / 2.0f;
		
						character->velocity.x = 0.0f;
					}
				}
			}
		}
	}

	// Move y
	character->position.y += character->velocity.y * delta;

	character->onGround = false;

	// Y collision
	for (int o = 0; o < lenObjects; o++) {
		if (character->position.x - character->size.x / 2.0f < objects[o].position.x + objects[o].size.x / 2.0f && character->position.x + character->size.x / 2.0f > objects[o].position.x - objects[o].size.x / 2.0f) {
			if (character->position.z - character->size.z / 2.0f < objects[o].position.z + objects[o].size.z / 2.0f && character->position.z + character->size.z / 2.0f > objects[o].position.z - objects[o].size.z / 2.0f) {
				if (character->position.y + character->size.y / 2.0f > objects[o].position.y - objects[o].size.y / 2.0f && character->position.y - character->size.y / 2.0f <= objects[o].position.y + objects[o].size.y / 2.0f) {
					// Falling onto the objects[o]
					if (character->velocity.y <= 0.0f) {
						character->position.y = objects[o].position.y + objects[o].size.y / 2.0f + character->size.y / 2.0f;
		
						character->velocity.y = 0.0f;
						character->onGround = true;
					}
		
					// Jumping into the bottom of the objects[o]
					else if (character->velocity.y > 0.0f) {
						character->position.y = objects[o].position.y - objects[o].size.y / 2.0f - character->size.y / 2.0f;
		
						character->velocity.y = 0.0f;
					}
				}
			}
		}
	}

	// Move z
	character->position.z += character->velocity.z * delta;

	// Z collision
	for (int o = 0; o < lenObjects; o++) {
		if (character->position.y - character->size.y / 2.0f < objects[o].position.y + objects[o].size.y / 2.0f - COLLISION_EPSILON && character->position.y + character->size.y / 2.0f > objects[o].position.y - objects[o].size.y / 2.0f + COLLISION_EPSILON) {
			if (character->position.x - character->size.x / 2.0f < objects[o].position.x + objects[o].size.x / 2.0f && character->position.x + character->size.x / 2.0f > objects[o].position.x - objects[o].size.x / 2.0f) {
				if (character->position.z + character->size.z / 2.0f > objects[o].position.z - objects[o].size.z / 2.0f && character->position.z - character->size.z / 2.0f < objects[o].position.z + objects[o].size.z / 2.0f) {
					if (character->velocity.z > 0.0f) {
						character->position.z = objects[o].position.z - objects[o].size.z / 2.0f - character->size.z / 2.0f;
		
						character->velocity.z = 0.0f;
					}
					else if (character->velocity.z < 0.0f) {
						character->position.z = objects[o].position.z + objects[o].size.z / 2.0f + character->size.z / 2.0f;
		
						character->velocity.z = 0.0f;
					}
				}
			}
		}
	}

	// Check collision with floor
	if (character->position.y <= 0.0f + (character->size.y / 2.0f)) {
		character->position.y = 0.0f + (character->size.y / 2.0f);
		character->onGround = true;

		if (character->velocity.y < 0.0f) character->velocity.y = 0.0f;
	}

	// Walking sounds
	float horizontalSpeed = sqrtf(character->velocity.x * character->velocity.x + character->velocity.z * character->velocity.z);
	
	if (character->onGround && horizontalSpeed > 0.01f) {
		character->footstepTimer -= delta;
	
		if (character->footstepTimer <= 0.0f) {
			PlayPositionalSound(walkSound, *camera, (Vector3){ character->position.x, character->position.y - character->size.y, character->position.z }, 7.0f);
	
			// Tune these numbers to taste.
			const float referenceSpeed = PLAYER_SPEED;
			const float referenceInterval = 0.45f;
	
			character->footstepTimer = referenceInterval * (referenceSpeed / horizontalSpeed);
		}
	}
	else {
		character->footstepTimer = 0.0f;
	}
	// ------------------------------------------

	// Camera
	// ------------------------------------------
	if (character->isHuman) {
		// Player camera stuff
		float eyeHeight = character->size.y / 4;
		camera->position = (Vector3){ character->position.x, character->position.y + eyeHeight, character->position.z };

		Vector3 forward = { cosf(character->pitch) * cosf(character->yaw), sinf(character->pitch), cosf(character->pitch) * sinf(character->yaw) };

		camera->target = Vector3Add(camera->position, forward);
	}
	// ------------------------------------------
}

// Delete a box from the boxes array
int DeleteBox(Box *boxes, int index, int lenBoxes) {
	// Shift boxes to account for deletion
	for (int i = index; i < lenBoxes - 1; i++) {
		boxes[i] = boxes[i + 1];
	}

	return lenBoxes - 1;
}

// Add a box to boxes array.
int AddBox(Box *boxes, int lenBoxes, Vector3 position, Color color, Model model, float health) {
	if (lenBoxes >= MAX_BOXES) {
		return lenBoxes;
	}

	boxes[lenBoxes] = (Box){position, (Vector3){ BOX_SIZE, BOX_SIZE, BOX_SIZE }, model, color, health};

	return lenBoxes + 1;
}

void DrawBoxes(Box boxes[], int lenBoxes) {
	for (int o = 0; o < lenBoxes; o++){
		DrawModel(boxes[o].model, boxes[o].position, BOX_SIZE, boxes[o].color);
	}
}

/*
void UpdatePickup(Pickup *pickup, Camera listener) {
	// Calculate angle
	pickup->spin += pickup->spinSpeed;
	if (pickup->spin > 360.0f) pickup->spin = 0.0f;

	// Calculate bounce position
	pickup->bounceTime += GetFrameTime();

	float t = pickup->bounceTime / pickup->bounceDuration;

	if (t >= 1.0f) t = 1.0f;	

	// Ease in/out
	float eased = (1.0f - cosf(t * PI)) * 0.5f;

	float startY;
	float endY;

	if (pickup->bounceUp) {
		startY = pickup->targetY - 0.1f;
		endY = pickup->targetY + 0.1f;
	}
	else {
		startY = pickup->targetY + 0.1f;
		endY = pickup->targetY - 0.1f;
	}

	pickup->position.y = startY + (endY - startY) * eased;

	if (pickup->bounceTime >= pickup->bounceDuration) {
		pickup->bounceTime = 0.0f;
		pickup->bounceUp = !pickup->bounceUp;
	}

	// Play idle sound
	UpdatePositionalSound(pickup->idleSound, listener, pickup->position, 0.1f);

	if (!IsSoundPlaying(pickup->idleSound)) {
		PlaySound(pickup->idleSound);
	}
}

void DrawPickup(Pickup pickup) {
	DrawModelEx(pickup.model, pickup.position, (Vector3){ 0.0f, 1.0f, 0.0f }, pickup.spin, (Vector3){ 1.8f, 1.8f, 1.8f }, WHITE);
	//DrawCubeWires(pickup.position, pickup.size.x, pickup.size.y, pickup.size.z, GREEN);
}
*/

void UpdatePickups(Pickup pickups[], int lenPickups, Camera listener) {
	for (int o = 0; o < lenPickups; o++){
		//UpdatePickup(&pickups[o], listener);

		// Calculate angle
		pickups[o].spin += pickups[o].spinSpeed;
		if (pickups[o].spin > 360.0f) pickups[o].spin = 0.0f;
	
		// Calculate bounce position
		pickups[o].bounceTime += GetFrameTime();
	
		float t = pickups[o].bounceTime / pickups[o].bounceDuration;
	
		if (t >= 1.0f) t = 1.0f;	
	
		// Ease in/out
		float eased = (1.0f - cosf(t * PI)) * 0.5f;
	
		float startY;
		float endY;
	
		if (pickups[o].bounceUp) {
			startY = pickups[o].targetY - 0.1f;
			endY = pickups[o].targetY + 0.1f;
		}
		else {
			startY = pickups[o].targetY + 0.1f;
			endY = pickups[o].targetY - 0.1f;
		}
	
		pickups[o].position.y = startY + (endY - startY) * eased;
	
		if (pickups[o].bounceTime >= pickups[o].bounceDuration) {
			pickups[o].bounceTime = 0.0f;
			pickups[o].bounceUp = !pickups[o].bounceUp;
		}
	
		// Play idle sound
		// TODO: This bit need to change to account for sound pooling
		UpdatePositionalSound(pickups[o].idleSoundPool[o], listener, pickups[o].position, 0.1f);
	
		if (!IsSoundPlaying(pickups[o].idleSoundPool[o])) {
			PlaySound(pickups[o].idleSoundPool[o]);
		}
	}
}

void DrawPickups(Pickup pickups[], int lenPickups) {
	for (int o = 0; o < lenPickups; o++) {
		DrawModelEx(pickups[o].model, pickups[o].position, (Vector3){ 0.0f, 1.0f, 0.0f }, pickups[o].spin, (Vector3){ 1.8f, 1.8f, 1.8f }, WHITE);
		//DrawCubeWires(pickups[o].position, pickups[o].size.x, pickups[o].size.y, pickups[o].size.z, GREEN);
	}
}

// Returns true if two boxes overlap.
bool BoxesOverlap(Box a, Box b) {
	float aMinX = a.position.x - a.size.x / 2.0f;
	float aMaxX = a.position.x + a.size.x / 2.0f;
	float aMinY = a.position.y - a.size.y / 2.0f;
	float aMaxY = a.position.y + a.size.y / 2.0f;
	float aMinZ = a.position.z - a.size.z / 2.0f;
	float aMaxZ = a.position.z + a.size.z / 2.0f;

	float bMinX = b.position.x - b.size.x / 2.0f;
	float bMaxX = b.position.x + b.size.x / 2.0f;
	float bMinY = b.position.y - b.size.y / 2.0f;
	float bMaxY = b.position.y + b.size.y / 2.0f;
	float bMinZ = b.position.z - b.size.z / 2.0f;
	float bMaxZ = b.position.z + b.size.z / 2.0f;

	return (
		aMinX < bMaxX && aMaxX > bMinX &&
		aMinY < bMaxY && aMaxY > bMinY &&
		aMinZ < bMaxZ && aMaxZ > bMinZ
	);
}

// Check whether a proposed box can be placed without overlapping an existing box.
bool CanPlaceBox(Box boxes[], int lenBoxes, Vector3 position, Vector3 size) {
	Box newBox = { position, size, (Model){ 0 }, WHITE, 0.0f };

	for (int i = 0; i < lenBoxes; i++) {
		if (BoxesOverlap(newBox, boxes[i])) {
			return false;
		}
	}

	return true;
}

// Determine which face of a box was hit by the ray. Returns the normal of that face.
Vector3 GetBoxHitNormal(BoundingBox box, Vector3 hitPoint) {
	float distances[6];

	distances[0] = fabsf(hitPoint.x - box.min.x); // -X
	distances[1] = fabsf(hitPoint.x - box.max.x); // +X
	distances[2] = fabsf(hitPoint.y - box.min.y); // -Y
	distances[3] = fabsf(hitPoint.y - box.max.y); // +Y
	distances[4] = fabsf(hitPoint.z - box.min.z); // -Z
	distances[5] = fabsf(hitPoint.z - box.max.z); // +Z

	int closestFace = 0;

	for (int i = 1; i < 6; i++) {
		if (distances[i] < distances[closestFace]) {
			closestFace = i;
		}
	}

	switch (closestFace) {
		case 0: return (Vector3){ -1.0f,  0.0f,  0.0f };
		case 1: return (Vector3){  1.0f,  0.0f,  0.0f };
		case 2: return (Vector3){  0.0f, -1.0f,  0.0f };
		case 3: return (Vector3){  0.0f,  1.0f,  0.0f };
		case 4: return (Vector3){  0.0f,  0.0f, -1.0f };
		case 5: return (Vector3){ 0.0f,  0.0f,  1.0f };
	}

	return (Vector3){ 0.0f, 1.0f, 0.0f };
}

int main(void) {
	// Initialization
	const int screenWidth = 800;
	const int screenHeight = 450;

	// Multi-sample Anti-Aliasing
	SetConfigFlags(FLAG_MSAA_4X_HINT);

	// Window
	InitWindow(screenWidth, screenHeight, "Josh's Raylib Sandbox :)");

	// Disable default raylib escape key functionality 
	SetExitKey(KEY_NULL);

	// Sounds
	InitAudioDevice();
	Sound fxPlaceBox = LoadSound("assets/audio/snap.ogg");
	Sound fxBreakBox = LoadSound("assets/audio/click.ogg");
	Sound fxHitBox = LoadSound("assets/audio/crack.ogg");
	Sound fxStep = LoadSound("assets/audio/step.ogg");
	Sound fxJump = LoadSound("assets/audio/whoosh.ogg");
	Sound fxLand = LoadSound("assets/audio/land.ogg");
	Sound fxUIHover = LoadSound("assets/audio/tick.ogg");
	Sound fxUIClick = LoadSound("assets/audio/accept.ogg");
	//Sound fxPickupPulse = LoadSound("assets/audio/low-synth-pulse.ogg");

	int lenPickupPulsePool = MAX_SOUND_POOL_SIZE;
	Sound pickupPulsePool[lenPickupPulsePool];
	for (int i = 0; i < lenPickupPulsePool; i++) {
		pickupPulsePool[i] = LoadSound("assets/audio/low-synth-pulse.ogg");
	}

	// Models
	Model donutModel = LoadModel("assets/models/donut.glb");
	donutModel.transform = MatrixRotateXYZ((Vector3){ 0.0f, 0.0f, 45.0f });

	GameScreen screen = MENU;

	bool gamePaused = false;
	bool exitWindow = false;

	// Main Menu buttons
	MenuButton newGameButton = { 0 };
	newGameButton.size = (Vector2){ 200, 50 };
	newGameButton.buttonText = "New Game";
	newGameButton.fontSize = 20;
	newGameButton.position = (Vector2){ (GetScreenWidth() / 2) - (newGameButton.size.x / 2), (GetScreenHeight() / 2) - (newGameButton.size.y / 2) - 25 };
	newGameButton.buttonColor = RAYWHITE;
	newGameButton.borderColor = GRAY;
	newGameButton.textColor = BLACK;
	newGameButton.boarderOffset = 5;
	newGameButton.shadow = true;
	newGameButton.hoverSoundFlag = false;

	MenuButton loadGameButton = { 0 };
	loadGameButton.size = (Vector2){ 200, 50 };
	loadGameButton.buttonText = "Load Game";
	loadGameButton.fontSize = 20;
	loadGameButton.position = (Vector2){ (GetScreenWidth() / 2) - (loadGameButton.size.x / 2), (GetScreenHeight() / 2) - (loadGameButton.size.y / 2) + 50 };
	loadGameButton.buttonColor = RAYWHITE;
	loadGameButton.borderColor = GRAY;
	loadGameButton.textColor = BLACK;
	loadGameButton.boarderOffset = 5;
	loadGameButton.shadow = true;
	loadGameButton.hoverSoundFlag = false;

	MenuButton quitGameButton = { 0 };
	quitGameButton.size = (Vector2){ 200, 50 };
	quitGameButton.buttonText = "Quit";
	quitGameButton.fontSize = 20;
	quitGameButton.position = (Vector2){ (GetScreenWidth() / 2) - (quitGameButton.size.x / 2), (GetScreenHeight() / 2) - (quitGameButton.size.y / 2) + 125 };
	quitGameButton.buttonColor = RAYWHITE;
	quitGameButton.borderColor = GRAY;
	quitGameButton.textColor = BLACK;
	quitGameButton.boarderOffset = 5;
	quitGameButton.shadow = true;
	quitGameButton.hoverSoundFlag = false;

	// Pause Menu buttons
	MenuButton resumeButton = { 0 };
	resumeButton.size = (Vector2){ 200, 50 };
	resumeButton.buttonText = "Resume";
	resumeButton.fontSize = 20;
	resumeButton.position = (Vector2){ (GetScreenWidth() / 2) - (resumeButton.size.x / 2), (GetScreenHeight() / 2) - (resumeButton.size.y / 2) - 25 };
	resumeButton.buttonColor = RAYWHITE;
	resumeButton.borderColor = GRAY;
	resumeButton.textColor = BLACK;
	resumeButton.boarderOffset = 5;
	resumeButton.shadow = true;
	resumeButton.hoverSoundFlag = false;

	MenuButton saveGameButton = { 0 };
	saveGameButton.size = (Vector2){ 200, 50 };
	saveGameButton.buttonText = "Save Game";
	saveGameButton.fontSize = 20;
	saveGameButton.position = (Vector2){ (GetScreenWidth() / 2) - (saveGameButton.size.x / 2), (GetScreenHeight() / 2) - (saveGameButton.size.y / 2) + 50 };
	saveGameButton.buttonColor = RAYWHITE;
	saveGameButton.borderColor = GRAY;
	saveGameButton.textColor = BLACK;
	saveGameButton.boarderOffset = 5;
	saveGameButton.shadow = true;
	saveGameButton.hoverSoundFlag = false;

	MenuButton quitToMenuButton = { 0 };
	quitToMenuButton.size = (Vector2){ 200, 50 };
	quitToMenuButton.buttonText = "Quit to menu";
	quitToMenuButton.fontSize = 20;
	quitToMenuButton.position = (Vector2){ (GetScreenWidth() / 2) - (quitToMenuButton.size.x / 2), (GetScreenHeight() / 2) - (quitToMenuButton.size.y / 2) + 125 };
	quitToMenuButton.buttonColor = RAYWHITE;
	quitToMenuButton.borderColor = GRAY;
	quitToMenuButton.textColor = BLACK;
	quitToMenuButton.boarderOffset = 5;
	quitToMenuButton.shadow = true;
	quitToMenuButton.hoverSoundFlag = false;

	// Create generic camera
	Camera3D genericCamera = { 0 };
	genericCamera.position = (Vector3){ -5.0f, 5.0f, 5.0f };
	genericCamera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
	genericCamera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
	genericCamera.fovy = 60.0f;
	genericCamera.projection = CAMERA_PERSPECTIVE;

	// Viewport for generic camera
	RenderTexture2D viewport = LoadRenderTexture(screenWidth / 4, screenHeight / 4);
	Rectangle viewportRect = { 0.0f, 0.0f, (float)viewport.texture.width, (float)-viewport.texture.height };

	// Create player camera
	Camera3D playerCamera = { 0 };
	playerCamera.position = (Vector3){ 0.0f, 0.0f, 10.0f };
	playerCamera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
	playerCamera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
	playerCamera.fovy = 60.0f;
	playerCamera.projection = CAMERA_PERSPECTIVE;

	// Create player
	Character player = { 0 };
	player.size = (Vector3){ 0.90f, 1.90f, 0.90f };
	player.position = (Vector3){ 0.0f, 0.0f, 0.0f };
	player.isHuman = true;
	player.speed = PLAYER_SPEED;
	player.jumpForce = PLAYER_JUMP_FORCE;
	player.yaw = 0.0f;
	player.pitch = 0.0f;
	player.model = LoadModelFromMesh(GenMeshCube(player.size.x, player.size.y, player.size.z));
	player.footstepTimer = 0.0f;
	player.jumpBoost = false;
	player.landFlag = false;

	// Create boxes
	Model boxModel = LoadModelFromMesh(GenMeshCube(BOX_SIZE, BOX_SIZE, BOX_SIZE));
	int lenBoxes = 7;
	Box boxes[MAX_BOXES];
	boxes[0] = (Box){(Vector3){ -1.0f, 0.5f, -4.0f }, (Vector3){ BOX_SIZE, BOX_SIZE, BOX_SIZE }, boxModel, BLUE, 100.0f};
	boxes[1] = (Box){(Vector3){ 1.0f, 0.5f, -2.0f }, (Vector3){ BOX_SIZE, BOX_SIZE, BOX_SIZE }, boxModel, PURPLE, 100.0f};
	boxes[2] = (Box){(Vector3){ -1.0f, 0.5f * 3.0f, 4.0f }, (Vector3){ BOX_SIZE, BOX_SIZE, BOX_SIZE }, boxModel, ORANGE, 100.0f};
	boxes[3] = (Box){(Vector3){ 1.0f, 0.5f, 4.0f }, (Vector3){ BOX_SIZE, BOX_SIZE, BOX_SIZE }, boxModel, GREEN, 100.0f};
	boxes[4] = (Box){(Vector3){ -3.0f, 0.5f * 4.0f, 4.0f }, (Vector3){ BOX_SIZE, BOX_SIZE, BOX_SIZE }, boxModel, PINK, 100.0f};
	boxes[5] = (Box){(Vector3){ -3.0f, 0.5f * 4.0f, 3.0f }, (Vector3){ BOX_SIZE, BOX_SIZE, BOX_SIZE }, boxModel, BROWN, 100.0f};
	boxes[6] = (Box){(Vector3){ -3.0f, 0.5f * 4.0f, 5.0f }, (Vector3){ BOX_SIZE, BOX_SIZE, BOX_SIZE }, boxModel, RED, 100.0f};

	// Create pickups
	Pickup donuts[MAX_DONUTS];
	int lenDonuts = 4;
	donuts[0] = (Pickup){ .position = (Vector3){ 2.0f, 0.95f, 4.0f }, .size = (Vector3){ 0.5f, 0.5f, 0.5f }, .model = donutModel, .spin = 0.0f, .spinSpeed = 1.0f, .targetY = 0.95f, .bounceUp = true, .bounceTime = 0.0f, .bounceDuration = 1.0f, .lenIdleSoundPool = lenPickupPulsePool };
	memcpy(donuts[0].idleSoundPool, pickupPulsePool, sizeof(pickupPulsePool));

	donuts[1] = (Pickup){ .position = (Vector3){ -3.0f, 3.95f, 4.0f }, .size = (Vector3){ 0.5f, 0.5f, 0.5f }, .model = donutModel, .spin = 0.0f, .spinSpeed = 1.0f, .targetY = 3.95f, .bounceUp = true, .bounceTime = 0.0f, .bounceDuration = 1.0f, .lenIdleSoundPool = lenPickupPulsePool };
	memcpy(donuts[1].idleSoundPool, pickupPulsePool, sizeof(pickupPulsePool));

	donuts[2] = (Pickup){ .position = (Vector3){ 2.0f, 0.95f, -5.0f }, .size = (Vector3){ 0.5f, 0.5f, 0.5f }, .model = donutModel, .spin = 0.0f, .spinSpeed = 1.0f, .targetY = 0.95f, .bounceUp = true, .bounceTime = 0.0f, .bounceDuration = 1.0f, .lenIdleSoundPool = lenPickupPulsePool };
	memcpy(donuts[2].idleSoundPool, pickupPulsePool, sizeof(pickupPulsePool));

	donuts[3] = (Pickup){ .position = (Vector3){ -5.0f, 0.95f, -5.0f }, .size = (Vector3){ 0.5f, 0.5f, 0.5f }, .model = donutModel, .spin = 0.0f, .spinSpeed = 1.0f, .targetY = 0.95f, .bounceUp = true, .bounceTime = 0.0f, .bounceDuration = 1.0f, .lenIdleSoundPool = lenPickupPulsePool };
	memcpy(donuts[3].idleSoundPool, pickupPulsePool, sizeof(pickupPulsePool));

	float gravity = 10.0f;
	float mouseSensitivity = 0.003f;

	RayCollision boxClickCollision = { 0 };
	Ray boxClickRay = { 0 };

	SetTargetFPS(60);

	// Main game loop
	while (!WindowShouldClose() && !exitWindow) {
		// Update
		// ------------------------------------------
		switch (screen) {
			case MENU: {
				// Menu buttons
				newGameButton.position = (Vector2){ (GetScreenWidth() / 2) - (newGameButton.size.x / 2), (GetScreenHeight() / 2) - (newGameButton.size.y / 2) - 25 };
				UpdateMenuButton(&newGameButton, fxUIHover, fxUIClick);
				if (newGameButton.clicked) {
					DisableCursor();
					screen = GAMEPLAY;
					newGameButton.clicked = false;
				}

				loadGameButton.position = (Vector2){ (GetScreenWidth() / 2) - (loadGameButton.size.x / 2), (GetScreenHeight() / 2) - (loadGameButton.size.y / 2) + 50 };
				UpdateMenuButton(&loadGameButton, fxUIHover, fxUIClick);
				if (loadGameButton.clicked) {
					if (LoadGameData("save-data/save001.json", &player, boxes, &lenBoxes, boxModel)) {
						TraceLog(LOG_INFO, "Save file loaded.");
					} else {
						TraceLog(LOG_ERROR, "Failed to load save file.");
					}
					loadGameButton.clicked = false;
				}

				quitGameButton.position = (Vector2){ (GetScreenWidth() / 2) - (quitGameButton.size.x / 2), (GetScreenHeight() / 2) - (quitGameButton.size.y / 2) + 125 };
				UpdateMenuButton(&quitGameButton, fxUIHover, fxUIClick);
				if (quitGameButton.clicked) {
					exitWindow = true;
					quitGameButton.clicked = false;
				}
			} break;

			case GAMEPLAY: {
				if (!gamePaused) {
					// Toggle Pause
					if (IsKeyPressed(KEY_ESCAPE)) {
						gamePaused = true;
						EnableCursor();
						break;
					}

					// Update player
					UpdateCharacter(&player, &playerCamera, mouseSensitivity, gravity, boxes, lenBoxes, fxStep, fxJump, fxLand);

					// Click on boxes
					if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
						Vector3 direction = Vector3Normalize(Vector3Subtract(playerCamera.target, playerCamera.position));
						boxClickRay = (Ray){ playerCamera.position, Vector3Normalize(direction) };

						float closestDistance = MAX_BOX_DELETE_DISTANCE;
						int closestBox = -1;

						// Check collision between boxClickRay and boxes.
						for (int o = 0; o < lenBoxes; o++) {
							boxClickCollision = GetRayCollisionBox(boxClickRay, (BoundingBox){(Vector3){ boxes[o].position.x - boxes[o].size.x / 2, boxes[o].position.y - boxes[o].size.y / 2, boxes[o].position.z - boxes[o].size.z / 2 }, (Vector3){ boxes[o].position.x + boxes[o].size.x / 2, boxes[o].position.y + boxes[o].size.y / 2, boxes[o].position.z + boxes[o].size.z / 2 }});

							if (boxClickCollision.hit && boxClickCollision.distance <= closestDistance) {
								closestDistance = boxClickCollision.distance;
								closestBox = o;
							}
						}

						if (closestBox != -1) {
							// Damage box
							boxes[closestBox].health -= 33.34f;
							boxes[closestBox].color = (Color){ boxes[closestBox].color.r * 0.5f, boxes[closestBox].color.g * 0.5f, boxes[closestBox].color.b * 0.5f, 255};
							PlayPositionalSound(fxHitBox, playerCamera, boxes[closestBox].position, 7.0f);

							// Delete box if it is out of health
							if (boxes[closestBox].health <= 0.0f) {
								TraceLog(LOG_INFO, "Attempting to delete boxes[%d]...", closestBox);

								lenBoxes = DeleteBox(boxes, closestBox, lenBoxes);
								PlayPositionalSound(fxBreakBox, playerCamera, boxes[closestBox].position, 7.0f);

								for (int i = 0; i < lenBoxes; i++) {
									TraceLog(LOG_INFO, "boxes[%d]: (%.2f, %.2f, %.2f)", i, boxes[i].position.x, boxes[i].position.y, boxes[i].position.z);
								}
							}
						}
					}

					// Place new box.
					if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
						// Create ray
						Vector3 direction = Vector3Normalize(Vector3Subtract(playerCamera.target, playerCamera.position));
						boxClickRay = (Ray){ playerCamera.position, direction };
					
						RayCollision closestCollision = { 0 };
						int closestBox = -1;
					
						// Find the closest box hit by the placement ray.
						float closestDistance = FLT_MAX;
					
						for (int o = 0; o < lenBoxes; o++) {
							BoundingBox boxBounds = { (Vector3){ boxes[o].position.x - boxes[o].size.x / 2.0f, boxes[o].position.y - boxes[o].size.y / 2.0f, boxes[o].position.z - boxes[o].size.z / 2.0f }, (Vector3){ boxes[o].position.x + boxes[o].size.x / 2.0f, boxes[o].position.y + boxes[o].size.y / 2.0f, boxes[o].position.z + boxes[o].size.z / 2.0f } };
					
							RayCollision collision = GetRayCollisionBox(boxClickRay, boxBounds);
					
							if (collision.hit && collision.distance < closestDistance) {
								closestDistance = collision.distance;
								closestCollision = collision;
								closestBox = o;
							}
						}

						// Determine where the new box should go.
						Vector3 newBoxPosition = { 0 };
						bool validPlacementSurface = false;
					
						if (closestBox != -1) {
							// We hit an existing box.
							BoundingBox hitBox = { (Vector3){ boxes[closestBox].position.x - boxes[closestBox].size.x / 2.0f, boxes[closestBox].position.y - boxes[closestBox].size.y / 2.0f, boxes[closestBox].position.z - boxes[closestBox].size.z / 2.0f }, (Vector3){ boxes[closestBox].position.x + boxes[closestBox].size.x / 2.0f, boxes[closestBox].position.y + boxes[closestBox].size.y / 2.0f, boxes[closestBox].position.z + boxes[closestBox].size.z / 2.0f } };
							Vector3 normal = GetBoxHitNormal(hitBox, closestCollision.point);
					
							// Put the new box directly beside the face that was hit.
							newBoxPosition = Vector3Add(boxes[closestBox].position, normal);
							validPlacementSurface = true;
						}
						// No box was hit. Try the ground instead.
						else {
							if (fabsf(boxClickRay.direction.y) > 0.0001f) {
								float distanceToGround = (0.0f - boxClickRay.position.y) / boxClickRay.direction.y;
					
								if (distanceToGround > 0.0f) {
									Vector3 groundPoint = Vector3Add(boxClickRay.position, Vector3Scale(boxClickRay.direction, distanceToGround));
									newBoxPosition = (Vector3){ groundPoint.x, BOX_SIZE / 2.0f, groundPoint.z };
									validPlacementSurface = true;
								}
							}
						}
					
						// Only continue if the ray actually hit a valid placement surface.
						if (validPlacementSurface) {
							Vector3 newBoxSize = { BOX_SIZE, BOX_SIZE, BOX_SIZE };
					
							// Check distance from player.
							float playerToBoxDistance = Vector3Distance(player.position, newBoxPosition);
					
							if (playerToBoxDistance <= MAX_BOX_PLACE_DISTANCE) {
								// Check that the new box doesn't overlap an existing box or the player.
								if (CanPlaceBox(boxes, lenBoxes, newBoxPosition, newBoxSize) && !CheckCollisionBoxes((BoundingBox){player.position, Vector3Add(player.position, player.size)}, (BoundingBox){ newBoxPosition, Vector3Add(newBoxPosition, newBoxSize) })) {
									TraceLog(LOG_INFO, "Attempting to create boxes[%d]...", lenBoxes);

									lenBoxes = AddBox(boxes, lenBoxes, newBoxPosition, GREEN, boxModel, 100.0f);
									PlayPositionalSound(fxPlaceBox, playerCamera, newBoxPosition, 7.0f);

									for (int i = 0; i < lenBoxes; i++) {
										TraceLog(LOG_INFO, "boxes[%d]: (%.2f, %.2f, %.2f)", i, boxes[i].position.x, boxes[i].position.y, boxes[i].position.z);
									}
								}
								else {
									TraceLog(LOG_INFO, "Cannot place box: position occupied.");
								}
							}
							else {
								TraceLog(LOG_INFO, "Box too far away: %.2f", playerToBoxDistance);
							}
						}

					}

					// Update pickups
					UpdatePickups(donuts, lenDonuts, playerCamera);
				}

				// Game Paused
				else {
					// Detect escape key press.
					if (IsKeyPressed(KEY_ESCAPE)) {
						DisableCursor();
						gamePaused = false;
					}

					// Buttons
					resumeButton.position = (Vector2){ (GetScreenWidth() / 2) - (resumeButton.size.x / 2), (GetScreenHeight() / 2) - (resumeButton.size.y / 2) - 25 };
					UpdateMenuButton(&resumeButton, fxUIHover, fxUIClick);
					if (resumeButton.clicked) {
						DisableCursor();
						gamePaused = false;
						resumeButton.clicked = false;
					}

					saveGameButton.position = (Vector2){ (GetScreenWidth() / 2) - (saveGameButton.size.x / 2), (GetScreenHeight() / 2) - (saveGameButton.size.y / 2) + 50 };
					UpdateMenuButton(&saveGameButton, fxUIHover, fxUIClick);
					if (saveGameButton.clicked) {
						if (SaveGameData("save-data/save001.json", &player, boxes, lenBoxes)) {
							TraceLog(LOG_INFO, "Game saved.");
						} else {
							TraceLog(LOG_ERROR, "Game save failed.");
						}
						saveGameButton.clicked = false;
					}

					quitToMenuButton.position = (Vector2){ (GetScreenWidth() / 2) - (quitToMenuButton.size.x / 2), (GetScreenHeight() / 2) - (quitToMenuButton.size.y / 2) + 125 };
					UpdateMenuButton(&quitToMenuButton, fxUIHover, fxUIClick);
					if (quitToMenuButton.clicked) {
						EnableCursor();
						screen = MENU;
						gamePaused = false;
						quitToMenuButton.clicked = false;
					}
				}
			} break;

			default: break;
		}
		// ------------------------------------------

		// Draw
		// ------------------------------------------
		switch (screen) {
			case MENU: {
				BeginDrawing();
					ClearBackground(BLACK);

					// Draw title
					DrawText("Josh's Raylib Sandbox", GetScreenWidth()/2 - MeasureText("Josh's Raylib Sandbox", 50)/2, GetScreenHeight()/2 - 150, 50, RAYWHITE);

					// Draw menu buttons
					DrawMenuButton(newGameButton);
					DrawMenuButton(loadGameButton);
					DrawMenuButton(quitGameButton);
				EndDrawing();
			} break;

			case GAMEPLAY: {
				// Construct viewport.
				BeginTextureMode(viewport);
					ClearBackground(BLACK);

					BeginMode3D(genericCamera);
						// Draw player
						DrawModelEx(player.model, player.position, (Vector3){ 0.0f, 1.0f, 0.0f }, -(player.yaw * 180.0f / PI), (Vector3){ 1, 1, 1 }, RED);

						// Draw boxes
						DrawBoxes(boxes, lenBoxes);

						// Draw pickups
						DrawPickups(donuts, lenDonuts);

						// Draw floor
						DrawFloor(20, 20, 1.0f, -0.5f * 20.0f * 1.0f - 0.5f * 1.0f, -0.5f * 20.0f * 1.0f - 0.5f * 1.0f);

						// Draw boxClickRay
						DrawRay(boxClickRay, RED);
					EndMode3D();
				EndTextureMode();

				BeginDrawing();
					ClearBackground(BLACK);

					BeginMode3D(playerCamera);
						// Draw floor 
						DrawFloor(20, 20, 1.0f, -0.5f * 20.0f * 1.0f - 0.5f * 1.0f, -0.5f * 20.0f * 1.0f - 0.5f * 1.0f);

						// Draw boxes
						DrawBoxes(boxes, lenBoxes);

						// Draw pickups
						DrawPickups(donuts, lenDonuts);

						// Draw boxClickRay
						DrawRay(boxClickRay, RED);
					EndMode3D();

					// Draw crosshair
					DrawRectangle((GetScreenWidth() / 2) - (8 / 2), (GetScreenHeight() / 2) - (2 / 2), 8, 2, DARKGREEN);
					DrawRectangle((GetScreenWidth() / 2) - (2 / 2), (GetScreenHeight() / 2) - (8 / 2), 2, 8, DARKGREEN);

					// Draw viewport
					DrawTextureRec(viewport.texture, viewportRect, (Vector2){ 10, 10 }, WHITE);

					// Draw fps
					DrawFPS(GetScreenWidth() - 100, 10);

					// Game Paused
					if (gamePaused) {
						// Draw pause backdrop
						DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.5f));

						DrawText("Paused", GetScreenWidth()/2 - MeasureText("Paused", 50)/2, GetScreenHeight()/2 - 150, 50, RAYWHITE);

						// Draw buttons
						DrawMenuButton(resumeButton);
						DrawMenuButton(saveGameButton);
						DrawMenuButton(quitToMenuButton);
					}
				EndDrawing();
			} break;

			default: break;
		}
		// ------------------------------------------
	}

	// De-initialization
	UnloadModel(player.model);
	UnloadModel(boxModel);
	UnloadModel(donutModel);

	UnloadRenderTexture(viewport);

	CloseAudioDevice();

	UnloadSound(fxPlaceBox);
	UnloadSound(fxBreakBox);
	UnloadSound(fxHitBox);
	UnloadSound(fxStep);
	UnloadSound(fxJump);
	UnloadSound(fxLand);
	UnloadSound(fxUIHover);
	UnloadSound(fxUIClick);
	//UnloadSound(fxPickupPulse);

	for (int i = 0; i < lenPickupPulsePool; i++) {
		UnloadSound(pickupPulsePool[i]);
	}

	CloseWindow();

	return 0;
}
