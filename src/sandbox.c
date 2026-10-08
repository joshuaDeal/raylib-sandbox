/*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*
My raylib sandbox :)

Use make to build. There is a makefile.

You can also try a gcc command like this:
`gcc -Wall -Wextra -g -o build/sandbox.bin src/sandbox.c src/buildtool.c src/character.c src/pickup.c src/savesystem.c src/soundsystem.c src/tespecial.c src/uisystem.c -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -lcjson`
*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*/

#include <raylib.h>
#include <raymath.h>
#include <float.h>
#include <cjson/cJSON.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "savesystem.h"
#include "uisystem.h"
#include "soundsystem.h"
#include "character.h"
#include "tespecial.h"
#include "buildtool.h"
#include "pickup.h"
#include "platform.h"

#define RLIGHTS_IMPLEMENTATION
#include "rlights.h"
#define GLSL_VERSION 330

typedef enum GameScreen { MENU, GAMEPLAY } GameScreen;
typedef enum MenuScreen { MAIN_MENU, LOAD_GAME } MenuScreen;
typedef enum PauseScreen { MAIN_PAUSE_MENU, SAVE_GAME } PauseScreen;

void DrawFloor(int slicesX, int slicesZ, float spacing, float xMin, float zMin) {
	// Draw lines parallel to X (vary Z)
	for (int zi = 0; zi <= slicesZ; zi++) {
		float z = zMin + (float)zi * spacing;
		DrawLine3D((Vector3){xMin, 0.0f, z}, (Vector3){xMin + slicesX * spacing, 0.0f, z}, WHITE);
	}

	// Draw lines parallel to Z (vary X)
	for (int xi = 0; xi <= slicesX; xi++) {
		float x = xMin + (float)xi * spacing;
		DrawLine3D((Vector3){x, 0.0f, zMin}, (Vector3){x, 0.0f, zMin + slicesZ * spacing}, WHITE);
	}
}

int main(void) {
	// Initialization
	const int screenWidth = 800;
	const int screenHeight = 450;

	// Multi-sample Anti-Aliasing
	SetConfigFlags(FLAG_MSAA_4X_HINT);

	// Window
	InitWindow(screenWidth, screenHeight, "Josh's Raylib Sandbox :)");

	// Disable default raylib escape key functionality 
	SetExitKey(KEY_NULL);

	// Shaders
	Shader basicLightingShader = LoadShader("assets/shaders/lighting.vs", "assets/shaders/lighting.fs");
	basicLightingShader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(basicLightingShader, "viewPos");

	// Save files
	FilePathList saveFiles = LoadSaveFiles();

	// Create save file buttons
	MenuButton fileButtons[(int)saveFiles.count];
	int yOffset = 0;
	for (int i = 0; i < (int)saveFiles.count; i++) {
		char *fileName = GetFileNameFromPath(saveFiles.paths[i]);

		if (fileName != NULL) {
			fileButtons[i].size = (Vector2){ 150, 38 };
			fileButtons[i].buttonText = fileName;
			fileButtons[i].fontSize = 15;
			fileButtons[i].position = (Vector2){ (GetScreenWidth() / 2) - (fileButtons[i].size.x / 2), 125 + yOffset };
			fileButtons[i].buttonColor = RAYWHITE;
			fileButtons[i].borderColor = GRAY;
			fileButtons[i].textColor = BLACK;
			fileButtons[i].boarderOffset = 5;
			fileButtons[i].shadow = true;
			fileButtons[i].hoverSoundFlag = false;
			fileButtons[i].clicked = false;

			yOffset += 40;
		}
	}

	// New save file button
	MenuButton newSaveButton = { 0 };
	newSaveButton.size = (Vector2){ 150, 38 };
	newSaveButton.buttonText = "New Save";
	newSaveButton.fontSize = 15;
	newSaveButton.position = (Vector2){ (GetScreenWidth() / 2) - (newSaveButton.size.x / 2), 125 + yOffset };
	newSaveButton.buttonColor = RAYWHITE;
	newSaveButton.borderColor = GRAY;
	newSaveButton.textColor = BLACK;
	newSaveButton.boarderOffset = 5;
	newSaveButton.shadow = true;
	newSaveButton.hoverSoundFlag = false;
	newSaveButton.clicked = false;

	// Sounds
	InitAudioDevice();
	Sound fxPlaceBox = LoadSound("assets/audio/snap.ogg");
	Sound fxBreakBox = LoadSound("assets/audio/break.ogg");
	Sound fxHitBox = LoadSound("assets/audio/crack.ogg");
	Sound fxStep = LoadSound("assets/audio/step.ogg");
	Sound fxJump = LoadSound("assets/audio/whoosh.ogg");
	Sound fxLand = LoadSound("assets/audio/land.ogg");
	Sound fxUIHover = LoadSound("assets/audio/tick.ogg");
	Sound fxUIClick = LoadSound("assets/audio/accept.ogg");
	Sound fxChangeColor = LoadSound("assets/audio/switch.ogg");

	// Sound pools
	SoundPool pickupPulsePool = createSoundPool("assets/audio/low-synth-pulse.ogg");
	SoundPool teShotPool = createSoundPool("assets/audio/38-shot.ogg");
	SoundPool reloadPool = createSoundPool("assets/audio/reload.ogg");
	SoundPool triggerPullPool = createSoundPool("assets/audio/trigger-pull.ogg");
	SoundPool getAmmoPool = createSoundPool("assets/audio/ammo.ogg");
	SoundPool pickupRespawnPool = createSoundPool("assets/audio/item-respawn.ogg");

	// Models
	Model donutModel = LoadModel("assets/models/donut.glb");
	// Use basic lighting shader for model's material shader
	for (int i = 0; i < donutModel.materialCount; i++) {
		donutModel.materials[i].shader = basicLightingShader;
	}
	donutModel.transform = MatrixRotateXYZ((Vector3){ 0.0f, 0.0f, 45.0f });

	Model teAmmoModel = LoadModel("assets/models/38-ammo.glb");
	// Use basic lighting shader for model's material shader
	for (int i = 0; i < teAmmoModel.materialCount; i++) {
		teAmmoModel.materials[i].shader = basicLightingShader;
	}

	Model teSpecialModel = LoadModel("assets/models/38-special.glb");
	for (int i = 0; i < teSpecialModel.materialCount; i++) {
		teSpecialModel.materials[i].shader = basicLightingShader;
	}

	Model bigMuzzleFlareModel = LoadModel("assets/models/big-muzzle-flare.glb");
	for (int i = 0; i < bigMuzzleFlareModel.materialCount; i++) {
		bigMuzzleFlareModel.materials[i].shader = basicLightingShader;
	}

	Model littleMuzzleFlareModel = LoadModel("assets/models/little-muzzle-flare.glb");
	for (int i = 0; i < littleMuzzleFlareModel.materialCount; i++) {
		littleMuzzleFlareModel.materials[i].shader = basicLightingShader;
	}


	// Ambient lighting
	int ambientLoc = GetShaderLocation(basicLightingShader, "ambient");
	SetShaderValue(basicLightingShader, ambientLoc, (float[4]){ 0.1f, 0.1f, 0.1f, 0.1f }, SHADER_UNIFORM_VEC4);

	GameScreen screen = MENU;
	MenuScreen menuScreen = MAIN_MENU;
	PauseScreen pauseScreen = MAIN_PAUSE_MENU;

	bool gamePaused = false;
	bool exitWindow = false;

	// Main Menu buttons
	MenuButton newGameButton = { 0 };
	newGameButton.size = (Vector2){ 200, 50 };
	newGameButton.buttonText = "New Game";
	newGameButton.fontSize = 20;
	newGameButton.position = (Vector2){ (GetScreenWidth() / 2) - (newGameButton.size.x / 2), (GetScreenHeight() / 2) - (newGameButton.size.y / 2) - 25 };
	newGameButton.buttonColor = RAYWHITE;
	newGameButton.borderColor = GRAY;
	newGameButton.textColor = BLACK;
	newGameButton.boarderOffset = 5;
	newGameButton.shadow = true;
	newGameButton.hoverSoundFlag = false;

	MenuButton loadGameButton = { 0 };
	loadGameButton.size = (Vector2){ 200, 50 };
	loadGameButton.buttonText = "Load Game";
	loadGameButton.fontSize = 20;
	loadGameButton.position = (Vector2){ (GetScreenWidth() / 2) - (loadGameButton.size.x / 2), (GetScreenHeight() / 2) - (loadGameButton.size.y / 2) + 50 };
	loadGameButton.buttonColor = RAYWHITE;
	loadGameButton.borderColor = GRAY;
	loadGameButton.textColor = BLACK;
	loadGameButton.boarderOffset = 5;
	loadGameButton.shadow = true;
	loadGameButton.hoverSoundFlag = false;

	MenuButton quitGameButton = { 0 };
	quitGameButton.size = (Vector2){ 200, 50 };
	quitGameButton.buttonText = "Quit";
	quitGameButton.fontSize = 20;
	quitGameButton.position = (Vector2){ (GetScreenWidth() / 2) - (quitGameButton.size.x / 2), (GetScreenHeight() / 2) - (quitGameButton.size.y / 2) + 125 };
	quitGameButton.buttonColor = RAYWHITE;
	quitGameButton.borderColor = GRAY;
	quitGameButton.textColor = BLACK;
	quitGameButton.boarderOffset = 5;
	quitGameButton.shadow = true;
	quitGameButton.hoverSoundFlag = false;

	// Load Game buttons
	MenuButton returnToMainButton = { 0 };
	returnToMainButton.size = (Vector2){ 200, 50 };
	returnToMainButton.buttonText = "<-";
	returnToMainButton.fontSize = 20;
	returnToMainButton.position = (Vector2){ (GetScreenWidth() / 100), (GetScreenHeight() / 100) };
	returnToMainButton.buttonColor = RAYWHITE;
	returnToMainButton.borderColor = GRAY;
	returnToMainButton.textColor = BLACK;
	returnToMainButton.boarderOffset = 5;
	returnToMainButton.shadow = true;
	returnToMainButton.hoverSoundFlag = false;

	// Pause Menu buttons
	MenuButton resumeButton = { 0 };
	resumeButton.size = (Vector2){ 200, 50 };
	resumeButton.buttonText = "Resume";
	resumeButton.fontSize = 20;
	resumeButton.position = (Vector2){ (GetScreenWidth() / 2) - (resumeButton.size.x / 2), (GetScreenHeight() / 2) - (resumeButton.size.y / 2) - 25 };
	resumeButton.buttonColor = RAYWHITE;
	resumeButton.borderColor = GRAY;
	resumeButton.textColor = BLACK;
	resumeButton.boarderOffset = 5;
	resumeButton.shadow = true;
	resumeButton.hoverSoundFlag = false;

	MenuButton saveGameButton = { 0 };
	saveGameButton.size = (Vector2){ 200, 50 };
	saveGameButton.buttonText = "Save Game";
	saveGameButton.fontSize = 20;
	saveGameButton.position = (Vector2){ (GetScreenWidth() / 2) - (saveGameButton.size.x / 2), (GetScreenHeight() / 2) - (saveGameButton.size.y / 2) + 50 };
	saveGameButton.buttonColor = RAYWHITE;
	saveGameButton.borderColor = GRAY;
	saveGameButton.textColor = BLACK;
	saveGameButton.boarderOffset = 5;
	saveGameButton.shadow = true;
	saveGameButton.hoverSoundFlag = false;

	MenuButton quitToMenuButton = { 0 };
	quitToMenuButton.size = (Vector2){ 200, 50 };
	quitToMenuButton.buttonText = "Quit to menu";
	quitToMenuButton.fontSize = 20;
	quitToMenuButton.position = (Vector2){ (GetScreenWidth() / 2) - (quitToMenuButton.size.x / 2), (GetScreenHeight() / 2) - (quitToMenuButton.size.y / 2) + 125 };
	quitToMenuButton.buttonColor = RAYWHITE;
	quitToMenuButton.borderColor = GRAY;
	quitToMenuButton.textColor = BLACK;
	quitToMenuButton.boarderOffset = 5;
	quitToMenuButton.shadow = true;
	quitToMenuButton.hoverSoundFlag = false;

	// Color picker
	int colorsPerCol = 8;
	int colorsPerRow = 8;
	Color colors[8][8] =
	{
		{
			(Color){ 250, 250, 250, 255 },
			(Color){ 220, 220, 220, 255 },
			(Color){ 190, 190, 190, 255 },
			(Color){ 160, 160, 160, 255 },
			(Color){ 125, 125, 125, 255 },
			(Color){  90,  90,  90, 255 },
			(Color){  55,  55,  55, 255 },
			(Color){  25,  25,  25, 255 }
		},
	
		{
			(Color){ 245, 150, 150, 255 },
			(Color){ 245, 190, 130, 255 },
			(Color){ 245, 225, 130, 255 },
			(Color){ 160, 220, 160, 255 },
			(Color){ 130, 220, 220, 255 },
			(Color){ 140, 175, 235, 255 },
			(Color){ 185, 150, 225, 255 },
			(Color){ 235, 150, 190, 255 }
		},
	
		{
			(Color){ 235, 110, 110, 255 },
			(Color){ 240, 165,  85, 255 },
			(Color){ 240, 210,  75, 255 },
			(Color){ 120, 200, 120, 255 },
			(Color){  75, 195, 195, 255 },
			(Color){ 105, 150, 225, 255 },
			(Color){ 160, 110, 210, 255 },
			(Color){ 225, 105, 160, 255 }
		},
	
		{
			(Color){ 220,  75,  75, 255 },
			(Color){ 230, 140,  50, 255 },
			(Color){ 230, 195,  40, 255 },
			(Color){  85, 180,  90, 255 },
			(Color){  45, 175, 175, 255 },
			(Color){  70, 125, 210, 255 },
			(Color){ 135,  80, 190, 255 },
			(Color){ 205,  70, 135, 255 }
		},
	
		{
			(Color){ 190,  50,  55, 255 },
			(Color){ 200, 110,  35, 255 },
			(Color){ 205, 165,  30, 255 },
			(Color){  60, 145,  70, 255 },
			(Color){  30, 145, 145, 255 },
			(Color){  50, 100, 180, 255 },
			(Color){ 110,  60, 165, 255 },
			(Color){ 180,  50, 115, 255 }
		},
	
		{
			(Color){ 155,  40,  45, 255 },
			(Color){ 165,  85,  30, 255 },
			(Color){ 170, 135,  25, 255 },
			(Color){  45, 115,  55, 255 },
			(Color){  25, 115, 115, 255 },
			(Color){  40,  80, 150, 255 },
			(Color){  85,  45, 135, 255 },
			(Color){ 145,  35,  90, 255 }
		},
	
		{
			(Color){ 115,  35,  40, 255 },
			(Color){ 125,  65,  25, 255 },
			(Color){ 130, 105,  20, 255 },
			(Color){  35,  85,  45, 255 },
			(Color){  20,  85,  85, 255 },
			(Color){  30,  60, 115, 255 },
			(Color){  65,  35, 100, 255 },
			(Color){ 105,  25,  65, 255 }
		},
	
		{
			(Color){  75,  25,  30, 255 },
			(Color){  85,  45,  20, 255 },
			(Color){  90,  70,  15, 255 },
			(Color){  25,  60,  30, 255 },
			(Color){  15,  60,  60, 255 },
			(Color){  20,  40,  80, 255 },
			(Color){  45,  25,  70, 255 },
			(Color){  70,  20,  45, 255 }
		}
	};
	int selectedColorX = 3;
	int selectedColorY = 3;
	bool showColorPicker = false;
	float colorPickerTimer = 0.0f;

	// Inventory HUD
	bool showInventory = false;
	float showInventoryTimer = 0.0f;

	// Create generic camera
	Camera3D genericCamera = { 0 };
	genericCamera.position = (Vector3){ -5.0f, 5.0f, 5.0f };
	genericCamera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
	genericCamera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
	genericCamera.fovy = 60.0f;
	genericCamera.projection = CAMERA_PERSPECTIVE;

	// Viewport for generic camera
	RenderTexture2D viewport = LoadRenderTexture(screenWidth / 4, screenHeight / 4);
	Rectangle viewportRect = { 0.0f, 0.0f, (float)viewport.texture.width, (float)-viewport.texture.height };

	// Create player camera
	Camera3D playerCamera = { 0 };
	playerCamera.position = (Vector3){ 0.0f, 0.0f, 10.0f };
	playerCamera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
	playerCamera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
	playerCamera.fovy = 60.0f;
	playerCamera.projection = CAMERA_PERSPECTIVE;

	// Create player
	Character player = { 0 };
	player.size = (Vector3){ 0.90f, 1.90f, 0.90f };
	player.position = (Vector3){ 0.0f, 0.0f, 0.0f };
	player.isHuman = true;
	player.speed = PLAYER_SPEED;
	player.jumpForce = PLAYER_JUMP_FORCE;
	player.yaw = 0.0f;
	player.pitch = 0.0f;
	player.model = LoadModelFromMesh(GenMeshCube(player.size.x, player.size.y, player.size.z));
	for (int i = 0; i < player.model.materialCount; i++) player.model.materials[i].shader = basicLightingShader;
	player.footstepTimer = 0.0f;
	player.jumpBoost = false;
	player.landFlag = false;
	for (int i = 0; i < NUM_INVENTORY_ITEMS; i++) player.inventory[i] = true;
	player.inventoryIndex = 0;

	// Create player weapons
	TeSpecial playerTeSpecial = {6, 6, 144, 144, 0.0f, -1, CreateLight(LIGHT_POINT, Vector3Zero(), Vector3Zero(), (Color){ 255, 214, 100, 255 }, 0.1f, basicLightingShader)};
	playerTeSpecial.flareLight.enabled = false;
	bool showTeSpecialHud = false;

	// Create boxes
	Model boxModel = LoadModelFromMesh(GenMeshCube(BOX_SIZE, BOX_SIZE, BOX_SIZE));
	for (int i = 0; i < boxModel.materialCount; i++) boxModel.materials[i].shader = basicLightingShader;
	int lenBoxes = 1;
	Box boxes[MAX_BOXES];
	boxes[0] = (Box){(Vector3){ 1.0f, 0.5f, 4.0f }, (Vector3){ BOX_SIZE, BOX_SIZE, BOX_SIZE }, boxModel, ORANGE, 100.0f};

	// Create platforms
	int lenPlatforms = 5;
	Platform platforms[MAX_PLATFORMS];
	platforms[0] = (Platform){ .position = (Vector3){ 0.0f, -0.25f, 0.0f }, .size = (Vector3){ 20.0f, 0.5f, 20.0f }, .color = GREEN, .solid = true };
	platforms[0].model = CreatePlatformModel(platforms[0].size, basicLightingShader);
	platforms[1] = (Platform){ .position = (Vector3){ 10.25f, 1.0f, 0.0f }, .size = (Vector3){ 0.5f, 2.0f, 20.0f }, .color = GRAY, .solid = true };
	platforms[1].model = CreatePlatformModel(platforms[1].size, basicLightingShader);
	platforms[2] = (Platform){ .position = (Vector3){ -10.25f, 1.0f, 0.0f }, .size = (Vector3){ 0.5f, 2.0f, 20.0f }, .color = GRAY, .solid = true };
	platforms[2].model = CreatePlatformModel(platforms[2].size, basicLightingShader);
	platforms[3] = (Platform){ .position = (Vector3){ 0.0f, 1.0f, 10.25f }, .size = (Vector3){ 20.0f, 2.0f, 0.5f }, .color = GRAY, .solid = true };
	platforms[3].model = CreatePlatformModel(platforms[3].size, basicLightingShader);
	platforms[4] = (Platform){ .position = (Vector3){ 0.0f, 1.0f, -10.25f }, .size = (Vector3){ 20.0f, 2.0f, 0.5f }, .color = GRAY, .solid = true };
	platforms[4].model = CreatePlatformModel(platforms[4].size, basicLightingShader);

	// Create pickups
	Pickup pickups[MAX_PICKUPS];
	int lenPickups = 6;
	pickups[0] = (Pickup){ .type = TESPECIAL_AMMO, .canRespawn = true, .respawnTime = 40.0f, .respawnTimer = 0.0f, .enabled = true, .position = (Vector3){ 2.0f, 0.95f, 4.0f }, .size = (Vector3){ 0.5f, 0.5f, 0.5f }, .model = teAmmoModel, .spin = 0.0f, .spinSpeed = 1.0f, .targetY = 0.95f, .bounceUp = true, .bounceTime = 0.0f, .bounceDuration = 1.0f };
	pickups[0].light = CreateLight(LIGHT_POINT, pickups[0].position, Vector3Zero(), (Color){ 255, 214, 100, 255 }, 0.3f, basicLightingShader);
	pickups[1] = (Pickup){ .type = TESPECIAL_AMMO, .canRespawn = true, .respawnTime = 40.0f, .respawnTimer = 0.0f, .enabled = true, .position = (Vector3){ -3.0f, 3.95f, 4.0f }, .size = (Vector3){ 0.5f, 0.5f, 0.5f }, .model = teAmmoModel, .spin = 0.0f, .spinSpeed = 1.0f, .targetY = 3.95f, .bounceUp = true, .bounceTime = 0.0f, .bounceDuration = 1.0f };
	pickups[1].light = CreateLight(LIGHT_POINT, pickups[1].position, Vector3Zero(), (Color){ 255, 214, 100, 255 }, 0.3f, basicLightingShader);
	pickups[2] = (Pickup){ .type = TESPECIAL_AMMO, .canRespawn = true, .respawnTime = 40.0f, .respawnTimer = 0.0f, .enabled = true, .position = (Vector3){ 2.0f, 0.95f, -5.0f }, .size = (Vector3){ 0.5f, 0.5f, 0.5f }, .model = teAmmoModel, .spin = 0.0f, .spinSpeed = 1.0f, .targetY = 0.95f, .bounceUp = true, .bounceTime = 0.0f, .bounceDuration = 1.0f };
	pickups[2].light = CreateLight(LIGHT_POINT, pickups[2].position, Vector3Zero(), (Color){ 255, 214, 100, 255 }, 0.3f, basicLightingShader);
	pickups[3] = (Pickup){ .type = DONUT, .canRespawn = true, .respawnTime = 40.0f, .respawnTimer = 0.0f, .enabled = true, .position = (Vector3){ -5.0f, 0.95f, -5.0f }, .size = (Vector3){ 0.5f, 0.5f, 0.5f }, .model = donutModel, .spin = 0.0f, .spinSpeed = 1.0f, .targetY = 0.95f, .bounceUp = true, .bounceTime = 0.0f, .bounceDuration = 1.0f };
	pickups[3].light = CreateLight(LIGHT_POINT, pickups[3].position, Vector3Zero(), (Color){ 255, 214, 100, 255 }, 0.3f, basicLightingShader);
	pickups[4] = (Pickup){ .type = DONUT, .canRespawn = true, .respawnTime = 40.0f, .respawnTimer = 0.0f, .enabled = true, .position = (Vector3){ -10.0f, 0.95f, 9.0f }, .size = (Vector3){ 0.5f, 0.5f, 0.5f }, .model = donutModel, .spin = 0.0f, .spinSpeed = 1.0f, .targetY = 0.95f, .bounceUp = true, .bounceTime = 0.0f, .bounceDuration = 1.0f };
	pickups[4].light = CreateLight(LIGHT_POINT, pickups[4].position, Vector3Zero(), (Color){ 255, 214, 100, 255 }, 0.3f, basicLightingShader);
	pickups[5] = (Pickup){ .type = DONUT, .canRespawn = true, .respawnTime = 40.0f, .respawnTimer = 0.0f, .enabled = true, .position = (Vector3){ -1.0f, 0.95f, 45.0f }, .size = (Vector3){ 0.5f, 0.5f, 0.5f }, .model = donutModel, .spin = 0.0f, .spinSpeed = 1.0f, .targetY = 0.95f, .bounceUp = true, .bounceTime = 0.0f, .bounceDuration = 1.0f };
	pickups[5].light = CreateLight(LIGHT_POINT, pickups[5].position, Vector3Zero(), (Color){ 255, 214, 100, 255 }, 0.3f, basicLightingShader);

	float gravity = 10.0f;
	float mouseSensitivity = 0.003f;

	RayCollision boxClickCollision = { 0 };
	Ray boxClickRay = { 0 };

	SetTargetFPS(60);

	// Main game loop
	while (!WindowShouldClose() && !exitWindow) {
		// Update
		// ------------------------------------------

		// Basic fullscreen toggle
		if (IsKeyPressed(KEY_F11)) ToggleFullscreen();

		switch (screen) {
			case MENU: {
				switch (menuScreen) {
					case MAIN_MENU: {
						// Menu buttons
						newGameButton.position = (Vector2){ (GetScreenWidth() / 2) - (newGameButton.size.x / 2), (GetScreenHeight() / 2) - (newGameButton.size.y / 2) - 25 };
						UpdateMenuButton(&newGameButton, fxUIHover, fxUIClick);
						if (newGameButton.clicked) {
							DisableCursor();
							screen = GAMEPLAY;
							newGameButton.clicked = false;
						}

						loadGameButton.position = (Vector2){ (GetScreenWidth() / 2) - (loadGameButton.size.x / 2), (GetScreenHeight() / 2) - (loadGameButton.size.y / 2) + 50 };
						UpdateMenuButton(&loadGameButton, fxUIHover, fxUIClick);
						if (loadGameButton.clicked) {
							menuScreen = LOAD_GAME;
							loadGameButton.clicked = false;
						}

						quitGameButton.position = (Vector2){ (GetScreenWidth() / 2) - (quitGameButton.size.x / 2), (GetScreenHeight() / 2) - (quitGameButton.size.y / 2) + 125 };
						UpdateMenuButton(&quitGameButton, fxUIHover, fxUIClick);
						if (quitGameButton.clicked) {
							exitWindow = true;
							quitGameButton.clicked = false;
						}
					} break;

					case LOAD_GAME: {
						// Buttons
						returnToMainButton.position = (Vector2){ 10, GetScreenHeight() - (returnToMainButton.size.y + 10) };
						UpdateMenuButton(&returnToMainButton, fxUIHover, fxUIClick);
						if (returnToMainButton.clicked) {
							menuScreen = MAIN_MENU;
							returnToMainButton.clicked = false;
						}

						// File buttons
						int yOffset = 0;
						for (int i = 0; i < (int)saveFiles.count; i++) {
							fileButtons[i].position = (Vector2){ (GetScreenWidth() / 2) - (fileButtons[i].size.x / 2), (GetScreenHeight() / 2 - 90) + yOffset };
							UpdateMenuButton(&fileButtons[i], fxUIHover, fxUIClick);
							if (fileButtons[i].clicked) {
								if (LoadGameData(saveFiles.paths[i], &player, boxes, &lenBoxes, boxModel)) {
									menuScreen = MAIN_MENU;
									screen = GAMEPLAY;
									DisableCursor();
									TraceLog(LOG_INFO, "Save file %s loaded.", saveFiles.paths[i]);
									fileButtons[i].clicked = false;
									break;
								} else {
									TraceLog(LOG_ERROR, "Failed to load save file: %s.", saveFiles.paths[i]);
								}
								
								fileButtons[i].clicked = false;
							}

							yOffset += 40;
						}

					} break;

					default: break;
				}
			} break;

			case GAMEPLAY: {
				if (!gamePaused) {
					// Toggle Pause
					if (IsKeyPressed(KEY_ESCAPE)) {
						gamePaused = true;
						EnableCursor();
						break;
					}

					// Update player
					UpdateCharacter(&player, &playerCamera, mouseSensitivity, gravity, boxes, lenBoxes, platforms, lenPlatforms, pickups, lenPickups, &playerTeSpecial, fxStep, fxJump, fxLand, &getAmmoPool, basicLightingShader);
					UpdateCharacterInventory(&player, &showInventory, &showInventoryTimer, boxes, &lenBoxes, &boxClickRay, boxClickCollision, &selectedColorY, &selectedColorX, &showColorPicker, &colorPickerTimer, colors, colorsPerRow, colorsPerCol, playerCamera, boxModel, fxHitBox, fxBreakBox, fxPlaceBox, fxChangeColor, &playerTeSpecial, &teShotPool, &reloadPool, &triggerPullPool, &showTeSpecialHud);

					// Update some lights
					UpdateLightValues(basicLightingShader, playerTeSpecial.flareLight);

					// Update pickups
					UpdatePickups(pickups, lenPickups, playerCamera, &pickupPulsePool, &pickupRespawnPool, basicLightingShader);
				}

				// Game Paused
				else {
					// Detect escape key press.
					if (IsKeyPressed(KEY_ESCAPE)) {
						DisableCursor();
						gamePaused = false;
						pauseScreen = MAIN_PAUSE_MENU;
					}

					switch (pauseScreen) {
						case MAIN_PAUSE_MENU: {

							// Buttons
							resumeButton.position = (Vector2){ (GetScreenWidth() / 2) - (resumeButton.size.x / 2), (GetScreenHeight() / 2) - (resumeButton.size.y / 2) - 25 };
							UpdateMenuButton(&resumeButton, fxUIHover, fxUIClick);
							if (resumeButton.clicked) {
								DisableCursor();
								gamePaused = false;
								resumeButton.clicked = false;
							}

							saveGameButton.position = (Vector2){ (GetScreenWidth() / 2) - (saveGameButton.size.x / 2), (GetScreenHeight() / 2) - (saveGameButton.size.y / 2) + 50 };
							UpdateMenuButton(&saveGameButton, fxUIHover, fxUIClick);
							if (saveGameButton.clicked) {
								pauseScreen = SAVE_GAME;
								saveGameButton.clicked = false;
							}

							quitToMenuButton.position = (Vector2){ (GetScreenWidth() / 2) - (quitToMenuButton.size.x / 2), (GetScreenHeight() / 2) - (quitToMenuButton.size.y / 2) + 125 };
							UpdateMenuButton(&quitToMenuButton, fxUIHover, fxUIClick);
							if (quitToMenuButton.clicked) {
								EnableCursor();
								screen = MENU;
								gamePaused = false;
								quitToMenuButton.clicked = false;
							}
						} break;

						case SAVE_GAME: {
							// Buttons
							returnToMainButton.position = (Vector2){ 10, GetScreenHeight() - (returnToMainButton.size.y + 10) };
							UpdateMenuButton(&returnToMainButton, fxUIHover, fxUIClick);
							if (returnToMainButton.clicked) {
								pauseScreen = MAIN_PAUSE_MENU;
								returnToMainButton.clicked = false;
							}

							// New save button
							int yOffset = 0;
							newSaveButton.position = (Vector2){ (GetScreenWidth() / 2) - (newSaveButton.size.x / 2), (GetScreenHeight() / 2 - 90) + yOffset };
							UpdateMenuButton(&newSaveButton, fxUIHover, fxUIClick);
							if (newSaveButton.clicked) {
								char *newSaveName = GetNewSaveFileName(&saveFiles);

								if (newSaveName != NULL) {
									if (SaveGameData(newSaveName, &player, boxes, lenBoxes)) {
										TraceLog(LOG_INFO, "Game saved to %s.", newSaveName);
										// TODO: We'll need to be able to update saveFiles and all the things dependant on it after creating a new save.
									}
									else {
										TraceLog(LOG_ERROR, "Failed to save game to %s", newSaveName);
									}

									free(newSaveName);
								}
								else {
									TraceLog(LOG_ERROR, "Game save failed: GetNewSaveFileName() returned NULL");
								}


								newSaveButton.clicked = false;
							}

							yOffset += 40;
							// File buttons
							for (int i = 0; i < (int)saveFiles.count; i++) {
								fileButtons[i].position = (Vector2){ (GetScreenWidth() / 2) - (fileButtons[i].size.x / 2), (GetScreenHeight() / 2 - 90) + yOffset };
								UpdateMenuButton(&fileButtons[i], fxUIHover, fxUIClick);
								if (fileButtons[i].clicked) {
									if (SaveGameData(saveFiles.paths[i], &player, boxes, lenBoxes)) {
										TraceLog(LOG_INFO, "Game saved to %s.", saveFiles.paths[i]);
										fileButtons[i].clicked = false;
										break;
									} else {
										TraceLog(LOG_ERROR, "Game save failed.");
									}
									
									fileButtons[i].clicked = false;
								}
	
								yOffset += 40;
							}
						} break;

						default: break;
					}
				}

				// Update shaders
				float playerCameraPos[3] = { playerCamera.position.x, playerCamera.position.y, playerCamera.position.z };
				SetShaderValue(basicLightingShader, basicLightingShader.locs[SHADER_LOC_VECTOR_VIEW], playerCameraPos, SHADER_UNIFORM_VEC3);

			} break;

			default: break;
		}
		// ------------------------------------------

		// Draw
		// ------------------------------------------
		switch (screen) {
			case MENU: {
				switch (menuScreen) {
					case MAIN_MENU: {
						BeginDrawing();
							ClearBackground(BLACK);

							// Draw title
							DrawText("Josh's Raylib Sandbox", GetScreenWidth()/2 - MeasureText("Josh's Raylib Sandbox", 50)/2, GetScreenHeight()/2 - 150, 50, RAYWHITE);

							// Draw menu buttons
							DrawMenuButton(newGameButton);
							DrawMenuButton(loadGameButton);
							DrawMenuButton(quitGameButton);
						EndDrawing();
					} break;

					case LOAD_GAME: {
						BeginDrawing();
							ClearBackground(BLACK);

							// Draw page title
							DrawText("Load Game", GetScreenWidth()/2 - MeasureText("Load Game", 50)/2, GetScreenHeight()/2 - 150, 50, RAYWHITE);

							// Draw files list buttons
							for (int i = 0; i < (int)saveFiles.count; i++) {
								DrawMenuButton(fileButtons[i]);
							}

							DrawMenuButton(returnToMainButton);
						EndDrawing();
					} break;

					default: break;
				}
			} break;

			case GAMEPLAY: {
				// Construct viewport.
				BeginTextureMode(viewport);
					ClearBackground(BLACK);

					BeginMode3D(genericCamera);

						BeginShaderMode(basicLightingShader);

							// Draw player
							DrawModelEx(player.model, player.position, (Vector3){ 0.0f, 1.0f, 0.0f }, -(player.yaw * 180.0f / PI), (Vector3){ 1, 1, 1 }, RED);

							// Draw player inventory item
							switch (player.inventoryIndex) {
								case INVENTORY_NOTHING: {
									break;
								}; break;
								case INVENTORY_BUILD_TOOL: {
									DrawItemBuildTool(playerCamera, boxModel, colors, selectedColorX, selectedColorY);
								}; break;
								case INVENTORY_TESPECIAL: {
									DrawItemTeSpecial(&playerTeSpecial, playerCamera, teSpecialModel, bigMuzzleFlareModel, littleMuzzleFlareModel);
								}; break;
								default: break;
							}

							// Draw boxes
							DrawBoxes(boxes, lenBoxes);

							// Draw platforms
							DrawPlatforms(platforms, lenPlatforms);

							// Draw pickups
							DrawPickups(pickups, lenPickups);

						EndShaderMode();

						// Draw floor
						DrawFloor(20, 20, 1.0f, -0.5f * 20.0f * 1.0f - 0.5f * 1.0f, -0.5f * 20.0f * 1.0f - 0.5f * 1.0f);

						// Draw boxClickRay
						DrawRay(boxClickRay, RED);

					EndMode3D();
				EndTextureMode();

				BeginDrawing();
					ClearBackground(BLACK);

					BeginMode3D(playerCamera);

						BeginShaderMode(basicLightingShader);
	
							// Draw boxes
							DrawBoxes(boxes, lenBoxes);

							// Draw platforms
							DrawPlatforms(platforms, lenPlatforms);
	
							// Draw pickups
							DrawPickups(pickups, lenPickups);

							// Draw player inventory item
							switch (player.inventoryIndex) {
								case INVENTORY_NOTHING: {
									break;
								}; break;
								case INVENTORY_BUILD_TOOL: {
									DrawItemBuildTool(playerCamera, boxModel, colors, selectedColorX, selectedColorY);
								}; break;
								case INVENTORY_TESPECIAL: {
									DrawItemTeSpecial(&playerTeSpecial, playerCamera, teSpecialModel, bigMuzzleFlareModel, littleMuzzleFlareModel);
								}
								default: break;
							}

						EndShaderMode();

						// Draw floor 
						//DrawFloor(20, 20, 1.0f, -0.5f * 20.0f * 1.0f - 0.5f * 1.0f, -0.5f * 20.0f * 1.0f - 0.5f * 1.0f);

						// Draw boxClickRay
						DrawRay(boxClickRay, RED);

					EndMode3D();

					// Draw crosshair
					DrawRectangle((GetScreenWidth() / 2) - (8 / 2), (GetScreenHeight() / 2) - (2 / 2), 8, 2, DARKGREEN);
					DrawRectangle((GetScreenWidth() / 2) - (2 / 2), (GetScreenHeight() / 2) - (8 / 2), 2, 8, DARKGREEN);

					// Draw viewport
					DrawTextureRec(viewport.texture, viewportRect, (Vector2){ 10, 10 }, WHITE);

					// Draw fps
					DrawFPS(GetScreenWidth() - 100, 10);

					// Color picker
					if (showColorPicker) {
						DrawColorPicker(colorsPerCol, colorsPerRow, selectedColorX, selectedColorY, colors);
					}

					// Inventory
					if (showInventory) {
						// Evaluate current item
						const char* itemText = TextFormat("%d", player.inventoryIndex);
						switch (player.inventoryIndex) {
							case INVENTORY_NOTHING: itemText = "None"; break;
							case INVENTORY_BUILD_TOOL: itemText = "Build Tool"; break;
							case INVENTORY_TESPECIAL: itemText = ".38 Special"; break;
							default: break;
						}

						DrawText(itemText, GetScreenWidth()/2 - MeasureText(itemText, 50)/2, GetScreenHeight()/2 - 150, 50, RAYWHITE);
					}

					// .38 Special ammunition
					if (showTeSpecialHud) {
						const char* ammoText = TextFormat("%d/%d", playerTeSpecial.roundsLoaded, playerTeSpecial.extraRounds);
						DrawText(ammoText, GetScreenWidth() - MeasureText(ammoText, 50) - (GetScreenWidth() / 100), GetScreenHeight() - 50, 50, RAYWHITE);
					}

					// Game Paused
					if (gamePaused) {
						// Draw pause backdrop
						DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.5f));

						switch (pauseScreen) {
							case MAIN_PAUSE_MENU: {
								DrawText("Paused", GetScreenWidth()/2 - MeasureText("Paused", 50)/2, GetScreenHeight()/2 - 150, 50, RAYWHITE);

								// Draw buttons
								DrawMenuButton(resumeButton);
								DrawMenuButton(saveGameButton);
								DrawMenuButton(quitToMenuButton);
							} break;

							case SAVE_GAME: {
								DrawText("Save Game", GetScreenWidth()/2 - MeasureText("Save Game", 50)/2, GetScreenHeight()/2 - 150, 50, RAYWHITE);

								// Draw new save button
								DrawMenuButton(newSaveButton);

								// Draw files list buttons
								for (int i = 0; i < (int)saveFiles.count; i++) {
									DrawMenuButton(fileButtons[i]);
								}

								DrawMenuButton(returnToMainButton);
							} break;

							default: break;
						}
					}
				EndDrawing();
			} break;

			default: break;
		}
		// ------------------------------------------
	}

	// De-initialization
	UnloadShader(basicLightingShader);

	UnloadModel(player.model);
	UnloadModel(boxModel);
	UnloadModel(donutModel);
	UnloadModel(teSpecialModel);
	UnloadModel(littleMuzzleFlareModel);
	UnloadModel(bigMuzzleFlareModel);
	UnloadModel(teAmmoModel);

	for (int i = 0; i < lenPlatforms; i++) UnloadModel(platforms[i].model);

	UnloadRenderTexture(viewport);

	CloseAudioDevice();

	UnloadSound(fxPlaceBox);
	UnloadSound(fxBreakBox);
	UnloadSound(fxHitBox);
	UnloadSound(fxStep);
	UnloadSound(fxJump);
	UnloadSound(fxLand);
	UnloadSound(fxUIHover);
	UnloadSound(fxUIClick);
	UnloadSound(fxChangeColor);

	UnloadSoundPool(pickupPulsePool);
	UnloadSoundPool(teShotPool);
	UnloadSoundPool(reloadPool);
	UnloadSoundPool(triggerPullPool);
	UnloadSoundPool(getAmmoPool);
	UnloadSoundPool(pickupRespawnPool);

	for (int i = 0; i < (int)saveFiles.count; i++) {
		free(fileButtons[i].buttonText);
	}

	UnloadDirectoryFiles(saveFiles);

	CloseWindow();

	return 0;
}
