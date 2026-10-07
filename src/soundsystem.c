#include "soundsystem.h"
#include <raymath.h>

void UpdatePositionalSound(Sound sound, Camera listener, Vector3 position, float maxDistance, float boost) {
	// Calculate direction and distance
	Vector3 direction = Vector3Subtract(position, listener.position);
	float distance = Vector3Length(direction);

	// Calculate attenuation
	float attenuation = 1.0f/(1.0f + (distance/maxDistance));
	attenuation = Clamp(attenuation, 0.0f, 1.0f);

	// Calculate normalized vectors
	Vector3 normalizedDirection = Vector3Normalize(direction);
	Vector3 forward = Vector3Normalize(Vector3Subtract(listener.target, listener.position));
	Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, listener.up));

	// Reduce volume for sounds behind the listener
	float dotProduct = Vector3DotProduct(forward, normalizedDirection);
	if (dotProduct < 0.0f) attenuation *= (1.0f + dotProduct*0.5f);

	// Set stereo panning
	float pan = 0.5f + 0.5f*Vector3DotProduct(normalizedDirection, right);

	// Apply changes to sound
	SetSoundVolume(sound, attenuation * boost);
	SetSoundPan(sound, pan);
}

void PlayPositionalSound(Sound sound, Camera listener, Vector3 position, float maxDistance, float boost) {
	UpdatePositionalSound(sound, listener, position, maxDistance, boost);
	PlaySound(sound);
}

void PlayPositionalSoundPoolSounds(SoundPool *soundPool, Camera listener, Vector3 position, float maxDistance, float boost) {
	// Release pool slots whose sounds have finished playing.
	for (int i = 0; i < soundPool->length; i++) {

		// Nothing is using this slot.
		if (soundPool->owner[i] == -1) continue;

		// The sound has finished, so the pool slot is available again.
		if (!IsSoundPlaying(soundPool->sounds[i])) {
			soundPool->owner[i] = -1;
		}
	}

	// Update the position/volume/panning of sounds that are currently assigned.
	for (int i = 0; i < soundPool->length; i++) {
		int soundIndex = soundPool->owner[i];

		// This sound is currently unused.
		if (soundIndex == -1) continue;

		UpdatePositionalSound(soundPool->sounds[i], listener, position, maxDistance, boost);
	}

	// Default to index 0.
	int freeSoundIndex = 0;

	// Find an unused sound in the pool.
	for (int i = 0; i < soundPool->length; i++) {
		if (soundPool->owner[i] == -1) {
			freeSoundIndex = i;
		}
	}

	// Set its initial positional properties before playing.
	UpdatePositionalSound(soundPool->sounds[freeSoundIndex], listener, position, maxDistance, boost);

	soundPool->owner[freeSoundIndex] = 0;

	PlaySound(soundPool->sounds[freeSoundIndex]);
}

void UpdateItemSounds(SoundPool *soundPool, Camera listener, float maxDistance, float boost) {
	// Get position of item
	Vector3 forward = Vector3Normalize(Vector3Subtract(listener.target, listener.position));
	Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, listener.up));
	Vector3 up = Vector3CrossProduct(right, forward);
	
	Vector3 itemOffset = Vector3Add(Vector3Scale(right, 0.5f), Vector3Scale(up, -0.25f));
	itemOffset = Vector3Add(itemOffset, Vector3Scale(forward, 1.0f));
	
	Vector3 itemPosition = Vector3Add(listener.position, itemOffset);

	// Release pool slots whose sounds have finished playing.
	for (int i = 0; i < soundPool->length; i++) {

		// Nothing is using this slot.
		if (soundPool->owner[i] == -1) continue;

		// The sound has finished, so the pool slot is available again.
		if (!IsSoundPlaying(soundPool->sounds[i])) {
			soundPool->owner[i] = -1;
		}
	}

	// Update the position/volume/panning of sounds that are currently assigned.
	for (int i = 0; i < soundPool->length; i++) {
		int soundIndex = soundPool->owner[i];

		// This sound is currently unused.
		if (soundIndex == -1) continue;

		UpdatePositionalSound(soundPool->sounds[i], listener, itemPosition, maxDistance, boost);
	}

	// Default to index 0.
	int freeSoundIndex = 0;

	// Find an unused sound in the pool.
	for (int i = 0; i < soundPool->length; i++) {
		if (soundPool->owner[i] == -1) {
			freeSoundIndex = i;
		}
	}

	// Set its initial positional properties before playing.
	UpdatePositionalSound(soundPool->sounds[freeSoundIndex], listener, itemPosition, maxDistance, boost);

	soundPool->owner[freeSoundIndex] = 0;

	PlaySound(soundPool->sounds[freeSoundIndex]);
}

// TODO: Look deeper into raylib sound functionality. See LoadSoundAlias().
SoundPool createSoundPool(char *sound) {
	SoundPool soundPool = { .length = MAX_SOUND_POOL_SIZE };
	for (int i = 0; i < soundPool.length; i++) {
		soundPool.sounds[i] = LoadSound(sound);
		soundPool.owner[i] = -1;
	}

	return soundPool;
}

void UnloadSoundPool(SoundPool soundPool) {
	for (int i = 0; i < soundPool.length; i++) {
		UnloadSound(soundPool.sounds[i]);
	}
}

void PlayUISound(Sound sound, float volume) {
	SetSoundVolume(sound, volume);
	SetSoundPan(sound, 0.0f);
	PlaySound(sound);
}
