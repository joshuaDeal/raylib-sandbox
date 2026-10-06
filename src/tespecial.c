#include "tespecial.h"
#include "buildtool.h"
#include <raymath.h>

void UpdateItemTeSpecial(TeSpecial *teSpecial, Box boxes[], int *lenBoxes, Ray *boxClickRay, RayCollision boxClickCollision, Camera playerCamera, Sound fxBreakBox, Sound fxHitBox, SoundPool *teShotPool, SoundPool *reloadPool, SoundPool *triggerPullPool) {
	// Shoot boxes
	if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && teSpecial->roundsLoaded > 0 && teSpecial->coolDownTimer <= 0.0f) {
		Vector3 direction = Vector3Normalize(Vector3Subtract(playerCamera.target, playerCamera.position));
		*boxClickRay = (Ray){ playerCamera.position, Vector3Normalize(direction) };

		float closestDistance = 30.0f;
		int closestBox = -1;

		// Check collision between boxClickRay and boxes.
		for (int o = 0; o < *lenBoxes; o++) {
			boxClickCollision = GetRayCollisionBox(*boxClickRay, (BoundingBox){(Vector3){ boxes[o].position.x - boxes[o].size.x / 2, boxes[o].position.y - boxes[o].size.y / 2, boxes[o].position.z - boxes[o].size.z / 2 }, (Vector3){ boxes[o].position.x + boxes[o].size.x / 2, boxes[o].position.y + boxes[o].size.y / 2, boxes[o].position.z + boxes[o].size.z / 2 }});

			if (boxClickCollision.hit && boxClickCollision.distance <= closestDistance) {
				closestDistance = boxClickCollision.distance;
				closestBox = o;
			}
		}

		if (closestBox != -1) {
			// Damage box
			boxes[closestBox].health -= 16.67f;
			boxes[closestBox].color = (Color){ boxes[closestBox].color.r * 0.75f, boxes[closestBox].color.g * 0.75f, boxes[closestBox].color.b * 0.75f, 255};
			PlayPositionalSound(fxHitBox, playerCamera, boxes[closestBox].position, 7.0f, 1.0f);

			// Delete box if it is out of health
			if (boxes[closestBox].health <= 0.0f) {
				TraceLog(LOG_INFO, "Attempting to delete boxes[%d]...", closestBox);

				*lenBoxes = DeleteBox(boxes, closestBox, *lenBoxes);
				PlayPositionalSound(fxBreakBox, playerCamera, boxes[closestBox].position, 7.0f, 1.0f);

				for (int i = 0; i < *lenBoxes; i++) {
					TraceLog(LOG_INFO, "boxes[%d]: (%.2f, %.2f, %.2f)", i, boxes[i].position.x, boxes[i].position.y, boxes[i].position.z);
				}
			}
		}

		teSpecial->roundsLoaded--;

		teSpecial->coolDownTimer = 0.17f;

		teSpecial->flareFrame = 0;

		// Play sound
		UpdateItemSounds(teShotPool, playerCamera, 1.0f, 3.0f);
	}
	else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && teSpecial->coolDownTimer <= 0.0f) {
		UpdateItemSounds(triggerPullPool, playerCamera, 1.0f, 1.0f);
	}

	// Reload
	if (IsKeyPressed(KEY_R)) {
		if (teSpecial->extraRounds > 0 && teSpecial->roundsLoaded != teSpecial->loadCapacity) {
			if (teSpecial->roundsLoaded + teSpecial->extraRounds <= 6) {
				teSpecial->roundsLoaded += teSpecial->extraRounds;
				teSpecial->extraRounds -= teSpecial->extraRounds;
			}
			else {
				int roundsToAdd = teSpecial->loadCapacity - teSpecial->roundsLoaded;

				teSpecial->roundsLoaded += roundsToAdd;
				teSpecial->extraRounds -= roundsToAdd;
			}

			teSpecial->coolDownTimer = 1.5f;

			// Play reload sound
			UpdateItemSounds(reloadPool, playerCamera, 1.0f, 1.0f);
		}
	}

	// Cool down timer
	if (teSpecial->coolDownTimer > 0.0f) {
		teSpecial->coolDownTimer -= GetFrameTime();
	}
	else if (teSpecial->coolDownTimer < 0.0f) teSpecial->coolDownTimer = 0.0f;

	// Muzzle flare frames
	if (teSpecial->flareFrame > -1 && teSpecial->flareFrame <= 2) {
		++teSpecial->flareFrame;
		TraceLog(LOG_INFO, "Falre frame: %d", teSpecial->flareFrame);
	}
	else teSpecial->flareFrame = -1;
}

void DrawItemTeSpecial(TeSpecial *teSpecial, Camera playerCamera, Model teSpecialModel, Model bigMuzzleFlareModel, Model littleMuzzleFlareModel) {
	Vector3 forward = Vector3Normalize(Vector3Subtract(playerCamera.target, playerCamera.position));
	Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, playerCamera.up));
	Vector3 up = Vector3CrossProduct(right, forward);
	
	Vector3 itemOffset = Vector3Add(Vector3Scale(right, 0.5f), Vector3Scale(up, -0.25f));
	itemOffset = Vector3Add(itemOffset, Vector3Scale(forward, 1.0f));
	
	Vector3 itemPosition = Vector3Add(playerCamera.position, itemOffset);

	// Orient the box to match the camera
	Matrix rotation = {
		right.x, up.x, -forward.x, 0.0f,
		right.y, up.y, -forward.y, 0.0f,
		right.z, up.z, -forward.z, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};

	Matrix oldTransform = teSpecialModel.transform;
	teSpecialModel.transform = MatrixMultiply(oldTransform, rotation);
	
	DrawModel(teSpecialModel, itemPosition, 1.0f, WHITE);

	teSpecialModel.transform = oldTransform;

	// Draw muzzle flare
	if (teSpecial->flareFrame == 1) {
		Vector3 forward = Vector3Normalize(Vector3Subtract(playerCamera.target, playerCamera.position));
		Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, playerCamera.up));
		Vector3 up = Vector3CrossProduct(right, forward);

		Vector3 itemOffset = Vector3Add(Vector3Scale(right, 0.5f), Vector3Scale(up, -0.2f));
		itemOffset = Vector3Add(itemOffset, Vector3Scale(forward, 1.5f));
		
		Vector3 itemPosition = Vector3Add(playerCamera.position, itemOffset);

		// Orient the box to match the camera
		Matrix rotation = {
			right.x, up.x, -forward.x, 0.0f,
			right.y, up.y, -forward.y, 0.0f,
			right.z, up.z, -forward.z, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		};

		Matrix oldTransform = bigMuzzleFlareModel.transform;
		bigMuzzleFlareModel.transform = MatrixMultiply(oldTransform, rotation);
		
		DrawModel(bigMuzzleFlareModel, itemPosition, 1.0f, WHITE);

		bigMuzzleFlareModel.transform = oldTransform;

		teSpecial->flareLight.position = itemPosition;
		teSpecial->flareLight.intensity = 0.1f;
		teSpecial->flareLight.enabled = true;
	}
	else if (teSpecial->flareFrame == 2) {
		Vector3 forward = Vector3Normalize(Vector3Subtract(playerCamera.target, playerCamera.position));
		Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, playerCamera.up));
		Vector3 up = Vector3CrossProduct(right, forward);

		Vector3 itemOffset = Vector3Add(Vector3Scale(right, 0.5f), Vector3Scale(up, -0.2f));
		itemOffset = Vector3Add(itemOffset, Vector3Scale(forward, 1.5f));
		
		Vector3 itemPosition = Vector3Add(playerCamera.position, itemOffset);

		// Orient the box to match the camera
		Matrix rotation = {
			right.x, up.x, -forward.x, 0.0f,
			right.y, up.y, -forward.y, 0.0f,
			right.z, up.z, -forward.z, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		};

		Matrix oldTransform = littleMuzzleFlareModel.transform;
		littleMuzzleFlareModel.transform = MatrixMultiply(oldTransform, rotation);
		
		DrawModel(littleMuzzleFlareModel, itemPosition, 1.0f, WHITE);

		littleMuzzleFlareModel.transform = oldTransform;

		teSpecial->flareLight.intensity = 0.05f;
	}
	else teSpecial->flareLight.enabled = false;
}
