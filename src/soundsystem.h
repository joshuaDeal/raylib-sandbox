#ifndef SOUNDSYSTEM_H
#define SOUNDSYSTEM_H

#include <raylib.h>

#define MAX_SOUND_POOL_SIZE 4

typedef struct SoundPool {
	Sound sounds[MAX_SOUND_POOL_SIZE];
	int owner[MAX_SOUND_POOL_SIZE];
	int length;
} SoundPool;

void UpdatePositionalSound(Sound sound, Camera listener, Vector3 position, float maxDistance, float boost);

void PlayPositionalSound(Sound sound, Camera listener, Vector3 position, float maxDistance, float boost);

void PlayPositionalSoundPoolSounds(SoundPool *soundPool, Camera listener, Vector3 position, float maxDistance, float boost);

void UpdateItemSounds(SoundPool *soundPool, Camera listener, float maxDistance, float boost);

SoundPool createSoundPool(char *sound);

void UnloadSoundPool(SoundPool soundPool);

void PlayUISound(Sound sound, float volume);

#endif
