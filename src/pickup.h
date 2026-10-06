#ifndef PICKUP_H
#define PICKUP_H

#define MAX_PICKUPS 100

#include <raylib.h>
#include "rlights.h"
#include "soundsystem.h"

typedef enum PickupTypes {
	DONUT = 0,
	TESPECIAL_AMMO = 1
} PickupTypes;

typedef struct Pickup {
	int type;
	Vector3 position;
	Vector3 size;
	Model model;
	float spin;
	float spinSpeed;
	float targetY;
	bool bounceUp;
	float bounceTime;
	float bounceDuration;
	Light light;
	bool canRespawn;
	float respawnTime;
	float respawnTimer;
	bool enabled;
} Pickup;

void UpdatePickupIdleSounds(SoundPool *soundPool, Pickup pickups[], int lenPickups, Camera listener);

void UpdatePickups(Pickup pickups[], int lenPickups, Camera listener, SoundPool *soundPool, Shader lightShader);

void DrawPickups(Pickup pickups[], int lenPickups);

#endif
