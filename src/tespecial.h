#ifndef TESPECIAL_H
#define TESPECIAL_H

#include "soundsystem.h"
#include <raylib.h>
#include "rlights.h"

typedef struct Box Box;

typedef struct TeSpecial {
	const int loadCapacity;
	int roundsLoaded;
	int extraRounds;
	const int extraRoundsCapacity;
	float coolDownTimer;
	int flareFrame;
	Light flareLight;
} TeSpecial;

void UpdateItemTeSpecial(TeSpecial *teSpecial, Box *boxes, int *lenBoxes, Ray *boxClickRay, RayCollision boxClickCollision, Camera playerCamera, Sound fxBreakBox, Sound fxHitBox, SoundPool *teShotPool, SoundPool *reloadPool, SoundPool *triggerPullPool);

void DrawItemTeSpecial(TeSpecial *teSpecial, Camera playerCamera, Model teSpecialModel, Model bigMuzzleFlareModel, Model littleMuzzleFlareModel);

#endif
