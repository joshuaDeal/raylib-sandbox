#ifndef EDITTOOL_H
#define EDITTOOL_H

#include "platform.h"
#include <raylib.h>

typedef enum EditMode {
	MOVE = 0,
	SCALE = 1,
	ROTATE = 2
} EditMode;

typedef struct EditTool {
	EditMode mode;
	bool showSelection;
} EditTool;

void UpdateItemEditTool(EditTool *editTool, Platform platforms[], int *lenPlatforms, Ray *clickRay, RayCollision clickCollision, Camera playerCamera);

#endif
