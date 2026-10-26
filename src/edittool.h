#ifndef EDITTOOL_H
#define EDITTOOL_H

#include "platform.h"
#include <raylib.h>

void UpdateItemEditTool(Platform platforms[], int *lenPlatforms, Ray *clickRay, RayCollision clickCollision, Camera playerCamera);

#endif
