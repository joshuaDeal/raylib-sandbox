#include "platform.h"

void DrawPlatforms(Platform *platforms, int lenPlatforms) {
	for (int i = 0; i < lenPlatforms; i++) {
		DrawModelEx(platforms[i].model, platforms[i].position, (Vector3){ 0.0f, 1.0f, 0.0f }, 0.0f, platforms[i].scale, platforms[i].color);
	}
}

void DrawPlatformSelection(Platform *platforms, int lenPlatforms) {
	for (int i = 0; i < lenPlatforms; i++) {
		if (platforms[i].selected) {
			DrawCubeWires(platforms[i].position, platforms[i].size.x + 0.2f, platforms[i].size.y + 0.2f, platforms[i].size.z + 0.2f, ORANGE);
		}
	}
}
