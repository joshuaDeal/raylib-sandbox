#include "buildtool.h"
#include <math.h>
#include "character.h"
#include <raymath.h>
#include <float.h>

void DrawColorPicker(int colorsPerCol, int colorsPerRow, int selectedColorX, int selectedColorY, Color colors[][8]) {
	int colorBoxSize = GetScreenWidth() / 50;
	int colorBoxSpacing = GetScreenWidth() / 400;
	int startingX = GetScreenWidth() - ((colorBoxSize + colorBoxSpacing)* colorsPerCol);
	int startingY = GetScreenHeight() - ((colorBoxSize + colorBoxSpacing)* colorsPerRow);
	int indicatorThickness = colorBoxSize / 10;
	for (int i = 0; i < colorsPerCol; i++) {
		for (int j = 0; j < colorsPerRow; j++) {
			// Draw selected indicator
			if (selectedColorX == i && selectedColorY == j) {
				DrawRectangle(startingX + (i * (colorBoxSize + colorBoxSpacing)) - indicatorThickness, startingY + (j * (colorBoxSize + colorBoxSpacing)) - indicatorThickness, colorBoxSize + (indicatorThickness * 2), colorBoxSize + (indicatorThickness * 2), WHITE);
			}

			// Draw color box
			DrawRectangle(startingX + (i * (colorBoxSize + colorBoxSpacing)), startingY + (j * (colorBoxSize + colorBoxSpacing)), colorBoxSize, colorBoxSize, colors[i][j]);
		}
	}
}

int DeleteBox(Box *boxes, int index, int lenBoxes) {
	// Shift boxes to account for deletion
	for (int i = index; i < lenBoxes - 1; i++) {
		boxes[i] = boxes[i + 1];
	}

	return lenBoxes - 1;
}

int AddBox(Box *boxes, int lenBoxes, Vector3 position, Color color, Model model, float health) {
	if (lenBoxes >= MAX_BOXES) {
		return lenBoxes;
	}

	boxes[lenBoxes] = (Box){position, (Vector3){ BOX_SIZE, BOX_SIZE, BOX_SIZE }, model, color, health};

	return lenBoxes + 1;
}

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

bool CanPlaceBox(Box boxes[], int lenBoxes, Vector3 position, Vector3 size) {
	Box newBox = { position, size, (Model){ 0 }, WHITE, 0.0f };

	for (int i = 0; i < lenBoxes; i++) {
		if (BoxesOverlap(newBox, boxes[i])) {
			return false;
		}
	}

	return true;
}

void UpdateItemBuildTool(Box boxes[], int *lenBoxes, Ray *boxClickRay, RayCollision boxClickCollision, int *selectedColorX, int *selectedColorY, bool *showColorPicker, float *colorPickerTimer, Color colors[][8], int colorsPerRow, int colorsPerCol, Camera playerCamera, Character player, Model boxModel, Sound fxHitBox, Sound fxBreakBox, Sound fxPlaceBox, Sound fxChangeColor) {
	// Click on boxes
	if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
		Vector3 direction = Vector3Normalize(Vector3Subtract(playerCamera.target, playerCamera.position));
		*boxClickRay = (Ray){ playerCamera.position, Vector3Normalize(direction) };

		float closestDistance = MAX_BOX_DELETE_DISTANCE;
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
			boxes[closestBox].health -= 33.34f;
			boxes[closestBox].color = (Color){ boxes[closestBox].color.r * 0.5f, boxes[closestBox].color.g * 0.5f, boxes[closestBox].color.b * 0.5f, 255};
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
	}

	// Place new box.
	if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
		// Create ray
		Vector3 direction = Vector3Normalize(Vector3Subtract(playerCamera.target, playerCamera.position));
		*boxClickRay = (Ray){ playerCamera.position, direction };
	
		RayCollision closestCollision = { 0 };
		int closestBox = -1;
	
		// Find the closest box hit by the placement ray.
		float closestDistance = FLT_MAX;
	
		for (int o = 0; o < *lenBoxes; o++) {
			BoundingBox boxBounds = { (Vector3){ boxes[o].position.x - boxes[o].size.x / 2.0f, boxes[o].position.y - boxes[o].size.y / 2.0f, boxes[o].position.z - boxes[o].size.z / 2.0f }, (Vector3){ boxes[o].position.x + boxes[o].size.x / 2.0f, boxes[o].position.y + boxes[o].size.y / 2.0f, boxes[o].position.z + boxes[o].size.z / 2.0f } };
	
			RayCollision collision = GetRayCollisionBox(*boxClickRay, boxBounds);
	
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
			if (fabsf(boxClickRay->direction.y) > 0.0001f) {
				float distanceToGround = (0.0f - boxClickRay->position.y) / boxClickRay->direction.y;
	
				if (distanceToGround > 0.0f) {
					Vector3 groundPoint = Vector3Add(boxClickRay->position, Vector3Scale(boxClickRay->direction, distanceToGround));
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
				if (CanPlaceBox(boxes, *lenBoxes, newBoxPosition, newBoxSize) && !CheckCollisionBoxes((BoundingBox){player.position, Vector3Add(player.position, player.size)}, (BoundingBox){ newBoxPosition, Vector3Add(newBoxPosition, newBoxSize) })) {
					TraceLog(LOG_INFO, "Attempting to create boxes[%d]...", *lenBoxes);

					*lenBoxes = AddBox(boxes, *lenBoxes, newBoxPosition, colors[*selectedColorX][*selectedColorY], boxModel, 100.0f);
					PlayPositionalSound(fxPlaceBox, playerCamera, newBoxPosition, 7.0f, 1.0f);

					for (int i = 0; i < *lenBoxes; i++) {
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

	// Color picker/selector
	if (*showColorPicker) {
		*colorPickerTimer += GetFrameTime();
	}

	if (*colorPickerTimer >= 3.0f) {
		*showColorPicker = false;
		*colorPickerTimer = 0.0f;
	}

	if (IsKeyPressed(KEY_DOWN)) {
		*showColorPicker = true;
		*colorPickerTimer = 0.0f;
		if (*selectedColorY < colorsPerRow - 1) {
			(*selectedColorY)++;
			PlayUISound(fxChangeColor, 0.5f);
		}
	}

	if (IsKeyPressed(KEY_UP)) {
		*showColorPicker = true;
		*colorPickerTimer = 0.0f;
		if (*selectedColorY > 0) {
			(*selectedColorY)--;
			PlayUISound(fxChangeColor, 0.5f);
		}
	}

	if (IsKeyPressed(KEY_LEFT)) {
		*showColorPicker = true;
		*colorPickerTimer = 0.0f;
		if (*selectedColorX > 0) {
			(*selectedColorX)--;
			PlayUISound(fxChangeColor, 0.5f);
		}
	}

	if (IsKeyPressed(KEY_RIGHT)) {
		*showColorPicker = true;
		*colorPickerTimer = 0.0f;
		if (*selectedColorX < colorsPerCol - 1) {
			(*selectedColorX)++;
			PlayUISound(fxChangeColor, 0.5f);
		}
	}
}

void DrawBoxes(Box boxes[], int lenBoxes) {
	for (int o = 0; o < lenBoxes; o++){
		DrawModel(boxes[o].model, boxes[o].position, BOX_SIZE, boxes[o].color);
	}
}

void DrawItemBuildTool(Camera playerCamera, Model boxModel, Color colors[][8], int selectedColorX, int selectedColorY) {
	Vector3 forward = Vector3Normalize(Vector3Subtract(playerCamera.target, playerCamera.position));
	Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, playerCamera.up));
	Vector3 up = Vector3CrossProduct(right, forward);
	
	Vector3 boxOffset = Vector3Add(Vector3Scale(right, 0.5f), Vector3Scale(up, -0.5f));
	boxOffset = Vector3Add(boxOffset, Vector3Scale(forward, 1.0f));
	
	Vector3 boxPosition = Vector3Add(playerCamera.position,	boxOffset);

	// Orient the box to match the camera
	Matrix rotation = {
		right.x, up.x, -forward.x, 0.0f,
		right.y, up.y, -forward.y, 0.0f,
		right.z, up.z, -forward.z, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};

	Matrix oldTransform = boxModel.transform;
	boxModel.transform = MatrixMultiply(oldTransform, rotation);
	
	DrawModel(boxModel, boxPosition, BOX_SIZE * 0.5f, colors[selectedColorX][selectedColorY]);

	boxModel.transform = oldTransform;
}
