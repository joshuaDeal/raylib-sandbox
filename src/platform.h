#ifndef PLATFORM_H
#define PLATFORM_H

#include <raylib.h>

#define MAX_PLATFORMS 1000

typedef struct Platform {
	Vector3 position;
	Vector3 size;
	Model model;
	Vector3 scale;
	Color color;
	bool solid;
} Platform;

void DrawPlatforms(Platform *platforms, int lenPlatforms);

#endif
