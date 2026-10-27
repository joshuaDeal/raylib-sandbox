#include "edittool.h"
#include <raymath.h>

void UpdateItemEditTool(EditTool *editTool, Platform platforms[], int *lenPlatforms, Ray *clickRay, RayCollision clickCollision, Camera playerCamera) {
	// Select a platform
	if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
		// Update ray
		Vector3 direction = Vector3Normalize(Vector3Subtract(playerCamera.target, playerCamera.position));
		*clickRay = (Ray){ playerCamera.position, Vector3Normalize(direction) };

		// Check collision
		float closestDistance = 1000.0;
		int closestHit = -1;

		for (int i = 0; i < *lenPlatforms; i++) {
			clickCollision = GetRayCollisionBox(*clickRay, (BoundingBox){(Vector3){ platforms[i].position.x - platforms[i].size.x / 2, platforms[i].position.y - platforms[i].size.y / 2, platforms[i].position.z - platforms[i].size.z / 2 }, (Vector3){ platforms[i].position.x + platforms[i].size.x / 2, platforms[i].position.y + platforms[i].size.y / 2, platforms[i].position.z + platforms[i].size.z / 2 }});

			if (clickCollision.hit && clickCollision.distance <= closestDistance) {
				closestDistance = clickCollision.distance;
				closestHit = i;
			}
		}

		// Deselect all platforms
		for (int i = 0; i < *lenPlatforms; i++) {
			platforms[i].selected = false;
		}

		// Select current platform if we have one
		if (closestHit != -1) {
			platforms[closestHit].selected = true;
		}
	}

	// Select a manipulation
	if (IsKeyPressed(KEY_Z)) editTool->mode = MOVE;
	if (IsKeyPressed(KEY_X)) editTool->mode = SCALE;
	if (IsKeyPressed(KEY_C)) editTool->mode = ROTATE;

	// Platform manipulations
	for (int i = 0; i < *lenPlatforms; i++) {
		if (platforms[i].selected) {
			switch (editTool->mode) {
				case MOVE: {
					if (IsKeyDown(KEY_UP)) platforms[i].position.x += 0.1f;
					if (IsKeyDown(KEY_DOWN)) platforms[i].position.x -= 0.1f;
					if (IsKeyDown(KEY_LEFT)) platforms[i].position.z += 0.1f;
					if (IsKeyDown(KEY_RIGHT)) platforms[i].position.z -= 0.1f;
					if (IsKeyDown(KEY_K)) platforms[i].position.y += 0.1f;
					if (IsKeyDown(KEY_J)) platforms[i].position.y -= 0.1f;
				}; break;

				case SCALE: {
					if (IsKeyDown(KEY_UP)) platforms[i].size.x += 0.1f;
					if (IsKeyDown(KEY_DOWN) && platforms[i].size.x > 0.1f) platforms[i].size.x -= 0.1f;
					if (IsKeyDown(KEY_LEFT)) platforms[i].size.z += 0.1f;
					if (IsKeyDown(KEY_RIGHT) && platforms[i].size.z > 0.1f) platforms[i].size.z -= 0.1f;
					if (IsKeyDown(KEY_K)) platforms[i].size.y += 0.1f;
					if (IsKeyDown(KEY_J) && platforms[i].size.y > 0.1f) platforms[i].size.y -= 0.1f;

					platforms[i].scale = platforms[i].size;
				}; break;

				case ROTATE: {

				}; break;

				default: break;
			}
		}
	}
	
	return;
}
