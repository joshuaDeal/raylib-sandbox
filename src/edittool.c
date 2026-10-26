#include "edittool.h"
#include <raymath.h>

void UpdateItemEditTool(Platform platforms[], int *lenPlatforms, Ray *clickRay, RayCollision clickCollision, Camera playerCamera) {
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

	return;
}
