#include "platform.h"

Model CreatePlatformModel(Vector3 size, Shader shader) {
	Model model = LoadModelFromMesh(GenMeshCube(size.x, size.y, size.z));

	for (int i = 0; i < model.materialCount; i++) model.materials[i].shader = shader;

	return model;
}

void DrawPlatforms(Platform *platforms, int lenPlatforms) {
	for (int i = 0; i < lenPlatforms; i++) {
		DrawModel(platforms[i].model, platforms[i].position, 1.0f, platforms[i].color);
	}
}
