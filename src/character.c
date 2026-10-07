#include "character.h"
#include "buildtool.h"
#include "tespecial.h"
#include "pickup.h"
#include <raymath.h>

void UpdateCharacterInventory(Character *character, bool *showInventory, float *showInventoryTimer, Box boxes[], int *lenBoxes, Ray *boxClickRay, RayCollision boxClickCollision, int *boxSelectedColorY, int *boxSelectedColorX, bool *showColorPicker, float *colorPickerTimer, Color colors[][8], int colorsPerRow, int colorsPerCol, Camera characterCamera, Model boxModel, Sound fxHitBox, Sound fxBreakBox, Sound fxPlaceBox, Sound fxChangeColor, TeSpecial *characterTeSpecial, SoundPool *teShotPool, SoundPool *reloadPool, SoundPool *triggerPullPool, bool *showTeSpecialHud) {
	// Update character inventory position
	float wheel = GetMouseWheelMove();
	if (wheel > 0) {
		if (character->inventoryIndex < NUM_INVENTORY_ITEMS - 1) {
			character->inventoryIndex++;
			*showInventory = true;
			*showInventoryTimer = 0.0f;
		} else {
			character->inventoryIndex = 0;
			*showInventory = true;
			*showInventoryTimer = 0.0f;
		}
	}
	else if (wheel < 0) {
		if (character->inventoryIndex > 0) {
			character->inventoryIndex--;
			*showInventory = true;
			*showInventoryTimer = 0.0f;
		} else {
			character->inventoryIndex = NUM_INVENTORY_ITEMS -1;
			*showInventory = true;
			*showInventoryTimer = 0.0f;
		}
	}

	// Update inventory item.
	if (character->inventoryIndex == INVENTORY_BUILD_TOOL && character->inventory[character->inventoryIndex]) {
		UpdateItemBuildTool(boxes, lenBoxes, boxClickRay, boxClickCollision, boxSelectedColorX, boxSelectedColorY, showColorPicker, colorPickerTimer, colors, colorsPerRow, colorsPerCol, characterCamera, *character, boxModel, fxHitBox, fxBreakBox, fxPlaceBox, fxChangeColor);
	} else if (*colorPickerTimer != 0.0f) {
		*colorPickerTimer = 0.0f;
		*showColorPicker = false;
	}

	if (character->inventoryIndex == INVENTORY_TESPECIAL && character->inventory[character->inventoryIndex]) {
		UpdateItemTeSpecial(characterTeSpecial, boxes, lenBoxes, boxClickRay, boxClickCollision, characterCamera, fxBreakBox, fxHitBox, teShotPool, reloadPool, triggerPullPool);
		*showTeSpecialHud = true;
	} else {
		*showTeSpecialHud = false;
	}

	if (*showInventory == true) {
		if (*showInventoryTimer <= 3.0f) {
			*showInventoryTimer += GetFrameTime();
		} else {
			*showInventory = false;
		}
	}
}

// TODO: Consider splitting this up into smaller functions. It's a lot.
void UpdateCharacter(Character *character, Camera3D *camera, float mouseSensitivity, float gravity, Box objects[], int lenObjects, Pickup pickups[], int lenPickups, TeSpecial *teSpecial, Sound walkSound, Sound jumpSound, Sound landSound, SoundPool *getAmmoPool, Shader lightShader) {
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
			PlayPositionalSound(jumpSound, *camera, (Vector3){ character->position.x, character->position.y - character->size.y, character->position.z }, 7.0f, 1.0f);
		}
	}

	// Landing sound
	if (character->onGround && character->landFlag) {
		PlayPositionalSound(landSound, *camera, (Vector3){ character->position.x, character->position.y - character->size.y, character->position.z }, 7.0f, 1.0f);
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

	// Collisions with pickups
	for (int i = 0; i < lenPickups; i++) {
		if (CheckCollisionBoxes((BoundingBox){(Vector3){ character->position.x - character->size.x / 2, character->position.y - character->size.y / 2, character->position.z - character->size.z / 2 }, (Vector3){ character->position.x + character->size.x / 2, character->position.y + character->size.y / 2, character->position.z + character->size.z / 2 }}, (BoundingBox){(Vector3){ pickups[i].position.x - pickups[i].size.x / 2, pickups[i].position.y - pickups[i].size.y / 2, pickups[i].position.z - pickups[i].size.z / 2 }, (Vector3){ pickups[i].position.x + pickups[i].size.x / 2, pickups[i].position.y + pickups[i].size.y / 2, pickups[i].position.z + pickups[i].size.z / 2 }}) && pickups[i].enabled) {
			TraceLog(LOG_INFO, "Colliding with pickup: %d", i);

			switch (pickups[i].type) {
				case DONUT: {
					break;
				} break;

				case TESPECIAL_AMMO: {
					if (teSpecial->extraRounds != teSpecial->extraRoundsCapacity) {
						// Give player ammo
						if (teSpecial->extraRounds + 24 < teSpecial->extraRoundsCapacity) teSpecial->extraRounds += 24;
						else teSpecial->extraRounds = teSpecial->extraRoundsCapacity;

						// Disable or delete pickup
						// We disable if canRespawn, otherwise, we delete
						pickups[i].enabled = false;
						pickups[i].light.enabled = false;
						UpdateLightValues(lightShader, pickups[i].light);

						// Play sound
						UpdateItemSounds(getAmmoPool, *camera, 1.0f, 1.0f);
					}
				} break;

				default: break;
			}
		}
	}

	// Walking sounds
	float horizontalSpeed = sqrtf(character->velocity.x * character->velocity.x + character->velocity.z * character->velocity.z);
	
	if (character->onGround && horizontalSpeed > 0.01f) {
		character->footstepTimer -= delta;
	
		if (character->footstepTimer <= 0.0f) {
			PlayPositionalSound(walkSound, *camera, (Vector3){ character->position.x, character->position.y - character->size.y, character->position.z }, 7.0f, 1.0f);
	
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
