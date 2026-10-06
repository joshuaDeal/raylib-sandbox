#ifndef BUILDTOOL_H
#define BUILDTOOL_H

#define MAX_BOXES 1000
#define BOX_SIZE 1.0f
#define MAX_BOX_PLACE_DISTANCE 3.0f
#define MAX_BOX_DELETE_DISTANCE 3.0f

#include <raylib.h>

typedef struct Character Character;

typedef struct Box {
	Vector3 position;
	Vector3 size;
	Model model;
	Color color;
	float health;
} Box;

void DrawColorPicker(int colorsPerCol, int colorsPerRow, int selectedColorX, int selectedColorY, Color colors[][8]);

int DeleteBox(Box *boxes, int index, int lenBoxes);

int AddBox(Box *boxes, int lenBoxes, Vector3 position, Color color, Model model, float health);

Vector3 GetBoxHitNormal(BoundingBox box, Vector3 hitPoint);

bool BoxesOverlap(Box a, Box b);

bool CanPlaceBox(Box boxes[], int lenBoxes, Vector3 position, Vector3 size);

void UpdateItemBuildTool(Box boxes[], int *lenBoxes, Ray *boxClickRay, RayCollision boxClickCollision, int *selectedColorX, int *selectedColorY, bool *showColorPicker, float *colorPickerTimer, Color colors[][8], int colorsPerRow, int colorsPerCol, Camera playerCamera, Character player, Model boxModel, Sound fxHitBox, Sound fxBreakBox, Sound fxPlaceBox, Sound fxChangeColor);

void DrawBoxes(Box boxes[], int lenBoxes);

void DrawItemBuildTool(Camera playerCamera, Model boxModel, Color colors[][8], int selectedColorX, int selectedColorY);

#endif
