#include "pickup.h"
#include <raymath.h>

void UpdatePickupIdleSounds(SoundPool *soundPool, Pickup pickups[], int lenPickups, Camera listener) {
	const float maxTriggeringDistance = 20.0f;

	// Release pool slots whose sounds have finished playing.
	for (int i = 0; i < soundPool->length; i++) {

		// Nothing is using this slot.
		if (soundPool->owner[i] == -1) continue;

		// The sound has finished, so the pool slot is available again.
		if (!IsSoundPlaying(soundPool->sounds[i])) {
			soundPool->owner[i] = -1;
		}
	}

	// Update the position/volume/panning of sounds that are currently assigned to pickups.
	for (int i = 0; i < soundPool->length; i++) {
		int pickupIndex = soundPool->owner[i];

		// This sound is currently unused.
		if (pickupIndex == -1) continue;

		UpdatePositionalSound(soundPool->sounds[i], listener, pickups[pickupIndex].position, 0.1f, 1.0f);
	}

	// Find the closest pickup that isn't currently represented by a sound in the pool.
	while (true) {
		int freeSoundIndex = -1;

		// Find an unused sound in the pool.
		for (int i = 0; i < soundPool->length; i++) {
			if (soundPool->owner[i] == -1) {
				freeSoundIndex = i;
				break;
			}
		}

		// There are no free sounds because every sound in the pool is currently being used.
		if (freeSoundIndex == -1)
			break;

		// Find the closest pickup that isn't already represented in the pool.
		int closestPickup = -1;
		float closestDistance = maxTriggeringDistance;

		for (int p = 0; p < lenPickups; p++) {
			// Determine whether this pickup already owns a sound from this pool.
			bool alreadyHasSound = false;
			for (int s = 0; s < soundPool->length; s++) {
				if (soundPool->owner[s] == p) {
					alreadyHasSound = true;
					break;
				}
			}

			// Don't consider pickups that already have a sound.
			if (alreadyHasSound) continue;

			// Don't consider pickups that are not enabled.
			if (!pickups[p].enabled) continue;

			// Calculate distance from the listener. We only care about distance here because this is being used to decide which pickup gets a scarce sound-pool slot.
			Vector3 direction = Vector3Subtract( pickups[p].position, listener.position);
			float distance = Vector3Length(direction);

			// Ignore pickups outside the sound's maximum range.
			if (distance > maxTriggeringDistance) continue;


			// Keep the closest eligible pickup.
			if (closestPickup == -1 || distance < closestDistance) {
				closestPickup = p;
				closestDistance = distance;
			}
		}


		// No pickup is close enough to use this sound.
		if (closestPickup == -1) break;


		// Assign the free sound to the closest eligible pickup.
		soundPool->owner[freeSoundIndex] = closestPickup;

		// Set its initial positional properties before playing.
		UpdatePositionalSound(soundPool->sounds[freeSoundIndex], listener, pickups[closestPickup].position, 0.1f, 1.0f);
		PlaySound(soundPool->sounds[freeSoundIndex]);
	}
}

void UpdatePickups(Pickup pickups[], int lenPickups, Camera listener, SoundPool *idleSoundPool, SoundPool *respawnSoundPool, Shader lightShader) {
	for (int o = 0; o < lenPickups; o++) {
		if (pickups[o].enabled) {
			// Calculate angle
			pickups[o].spin += pickups[o].spinSpeed;
			if (pickups[o].spin > 360.0f) pickups[o].spin = 0.0f;
		
			// Calculate bounce position
			pickups[o].bounceTime += GetFrameTime();
		
			float t = pickups[o].bounceTime / pickups[o].bounceDuration;
		
			if (t >= 1.0f) t = 1.0f;	
		
			// Ease in/out
			float eased = (1.0f - cosf(t * PI)) * 0.5f;
		
			float startY;
			float endY;
		
			if (pickups[o].bounceUp) {
				startY = pickups[o].targetY - 0.1f;
				endY = pickups[o].targetY + 0.1f;
			}
			else {
				startY = pickups[o].targetY + 0.1f;
				endY = pickups[o].targetY - 0.1f;
			}
		
			pickups[o].position.y = startY + (endY - startY) * eased;
		
			if (pickups[o].bounceTime >= pickups[o].bounceDuration) {
				pickups[o].bounceTime = 0.0f;
				pickups[o].bounceUp = !pickups[o].bounceUp;
			}

			// Update lights
			UpdateLightValues(lightShader, pickups[o].light);
		} else if (pickups[o].canRespawn) {
			pickups[o].respawnTimer += GetFrameTime();

			if (pickups[o].respawnTimer >= pickups[o].respawnTime) {
				pickups[o].enabled = true;
				pickups[o].light.enabled = true;
				pickups[o].respawnTimer = 0.0f;

				// Play sound
				PlayPositionalSoundPoolSounds(respawnSoundPool, listener, pickups[o].position, 0.2f, 1.0f);
			}
		}

		if (!pickups[o].enabled) {
			// Find out if disabled pickup has a sound pool sound assigned.
			for (int i = 0; i < idleSoundPool->length; i++) {
				if (idleSoundPool->owner[i] == o) {
					// If that sound is playing, stop it.
					if (IsSoundPlaying(idleSoundPool->sounds[i])) {
						StopSound(idleSoundPool->sounds[i]);
					}
				}
			}
		}
	}

	// Play idle sounds
	UpdatePickupIdleSounds(idleSoundPool, pickups, lenPickups, listener);
}

void DrawPickups(Pickup pickups[], int lenPickups) {
	for (int o = 0; o < lenPickups; o++) {
		if (pickups[o].enabled) {
			DrawModelEx(pickups[o].model, pickups[o].position, (Vector3){ 0.0f, 1.0f, 0.0f }, pickups[o].spin, (Vector3){ 1.8f, 1.8f, 1.8f }, WHITE);
			//DrawCubeWires(pickups[o].position, pickups[o].size.x, pickups[o].size.y, pickups[o].size.z, GREEN);
		}
	}
}
