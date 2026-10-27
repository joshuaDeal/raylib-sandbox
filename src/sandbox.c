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
#include "console.h"
#include "gamestate.h"
#include "edittool.h"

#define RLIGHTS_IMPLEMENTATION
#include "rlights.h"
#define GLSL_VERSION 330

typedef enum GameScreen { MENU, GAMEPLAY } GameScreen;
typedef enum MenuScreen { MAIN_MENU, LOAD_GAME } MenuScreen;
typedef enum PauseScreen { MAIN_PAUSE_MENU, SAVE_GAME } PauseScreen;

// Console for trace log
static Console *consoleForTraceLog;

static void ConsoleTraceLogCallback(int logLevel, const char *text, va_list args) {
	char message[CONSOLE_LOG_TEXT_SIZE];
	vsnprintf(message, sizeof(message), text, args);

	AddConsoleLog(consoleForTraceLog, logLevel, message);
}

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
	int screenWidth = 800;
	int screenHeight = 450;

	// Multi-sample Anti-Aliasing
	SetConfigFlags(FLAG_MSAA_4X_HINT);

	// Window
	InitWindow(screenWidth, screenHeight, "Josh's Raylib Sandbox :)");

	// Disable default raylib escape key functionality 
	SetExitKey(KEY_NULL);

	// Console
	SetTraceLogLevel(LOG_ALL);
	Console console = (Console){ 0 };
	console.enabled = true;
	console.visible = false;
	consoleForTraceLog = &console;
	SetTraceLogCallback(ConsoleTraceLogCallback);

	// GameState
	GameState gameState = { 0 };

	// Shaders
	gameState.basicLightingShader = LoadShader("assets/shaders/lighting.vs", "assets/shaders/lighting.fs");
	gameState.basicLightingShader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(gameState.basicLightingShader, "viewPos");

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
			fileButtons[i].position = (Vector2){ (screenWidth / 2) - (fileButtons[i].size.x / 2), 125 + yOffset };
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
	newSaveButton.position = (Vector2){ (screenWidth / 2) - (newSaveButton.size.x / 2), 125 + yOffset };
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
	gameState.donutModel = LoadModel("assets/models/donut.glb");
	// Use basic lighting shader for model's material shader
	for (int i = 0; i < gameState.donutModel.materialCount; i++) {
		gameState.donutModel.materials[i].shader = gameState.basicLightingShader;
	}
	gameState.donutModel.transform = MatrixRotateXYZ((Vector3){ 0.0f, 0.0f, 45.0f });

	gameState.teAmmoModel = LoadModel("assets/models/38-ammo.glb");
	// Use basic lighting shader for model's material shader
	for (int i = 0; i < gameState.teAmmoModel.materialCount; i++) {
		gameState.teAmmoModel.materials[i].shader = gameState.basicLightingShader;
	}

	Model teSpecialModel = LoadModel("assets/models/38-special.glb");
	for (int i = 0; i < teSpecialModel.materialCount; i++) {
		teSpecialModel.materials[i].shader = gameState.basicLightingShader;
	}

	Model bigMuzzleFlareModel = LoadModel("assets/models/big-muzzle-flare.glb");
	for (int i = 0; i < bigMuzzleFlareModel.materialCount; i++) {
		bigMuzzleFlareModel.materials[i].shader = gameState.basicLightingShader;
	}

	Model littleMuzzleFlareModel = LoadModel("assets/models/little-muzzle-flare.glb");
	for (int i = 0; i < littleMuzzleFlareModel.materialCount; i++) {
		littleMuzzleFlareModel.materials[i].shader = gameState.basicLightingShader;
	}

	gameState.playerModel = LoadModel("assets/models/mannequin.glb");
	for (int i = 0; i < gameState.playerModel.materialCount; i++) {
		gameState.playerModel.materials[i].shader = gameState.basicLightingShader;
	}


	// Ambient lighting
	int ambientLoc = GetShaderLocation(gameState.basicLightingShader, "ambient");
	SetShaderValue(gameState.basicLightingShader, ambientLoc, (float[4]){ 0.1f, 0.1f, 0.1f, 0.1f }, SHADER_UNIFORM_VEC4);

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
	newGameButton.position = (Vector2){ (screenWidth / 2) - (newGameButton.size.x / 2), (screenHeight / 2) - (newGameButton.size.y / 2) - 25 };
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
	loadGameButton.position = (Vector2){ (screenWidth / 2) - (loadGameButton.size.x / 2), (screenHeight / 2) - (loadGameButton.size.y / 2) + 50 };
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
	quitGameButton.position = (Vector2){ (screenWidth / 2) - (quitGameButton.size.x / 2), (screenHeight / 2) - (quitGameButton.size.y / 2) + 125 };
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
	returnToMainButton.position = (Vector2){ (screenWidth / 100), (screenHeight / 100) };
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
	resumeButton.position = (Vector2){ (screenWidth / 2) - (resumeButton.size.x / 2), (screenHeight / 2) - (resumeButton.size.y / 2) - 25 };
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
	saveGameButton.position = (Vector2){ (screenWidth / 2) - (saveGameButton.size.x / 2), (screenHeight / 2) - (saveGameButton.size.y / 2) + 50 };
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
	quitToMenuButton.position = (Vector2){ (screenWidth / 2) - (quitToMenuButton.size.x / 2), (screenHeight / 2) - (quitToMenuButton.size.y / 2) + 125 };
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
	gameState.genericCamera = (Camera3D){ 0 };
	gameState.genericCamera.position = (Vector3){ -5.0f, 5.0f, 5.0f };
	gameState.genericCamera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
	gameState.genericCamera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
	gameState.genericCamera.fovy = 60.0f;
	gameState.genericCamera.projection = CAMERA_PERSPECTIVE;

	// Viewport for generic camera
	RenderTexture2D viewport = LoadRenderTexture(screenWidth / 4, screenHeight / 4);
	Rectangle viewportRect = { 0.0f, 0.0f, (float)viewport.texture.width, (float)-viewport.texture.height };

	// Create player camera
	gameState.playerCamera = (Camera3D){ 0 };
	gameState.playerCamera.position = (Vector3){ 0.0f, 0.0f, 10.0f };
	gameState.playerCamera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
	gameState.playerCamera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
	gameState.playerCamera.fovy = 60.0f;
	gameState.playerCamera.projection = CAMERA_PERSPECTIVE;

	// Create characters. 
	gameState.lenCharacters = 2;

	// Player
	gameState.characters[0] = (Character){ 0 }; 
	gameState.characters[0].size = (Vector3){ 0.90f, 1.90f, 0.90f };
	gameState.characters[0].position = (Vector3){ 0.0f, 0.0f, 0.0f };
	gameState.characters[0].isHuman = true;
	gameState.characters[0].isPlayer = true;
	gameState.characters[0].speed = PLAYER_SPEED;
	gameState.characters[0].jumpForce = PLAYER_JUMP_FORCE;
	gameState.characters[0].yaw = 0.0f;
	gameState.characters[0].pitch = 0.0f;
	gameState.characters[0].model = gameState.playerModel;
	gameState.characters[0].footstepTimer = 0.0f;
	gameState.characters[0].jumpBoost = false;
	gameState.characters[0].landFlag = false;
	for (int i = 0; i < NUM_INVENTORY_ITEMS; i++) gameState.characters[0].inventory[i] = true;
	gameState.characters[0].inventoryIndex = 0;
	gameState.characters[0].health = 100.0f;
	gameState.characters[0].color = RED;
	gameState.characters[0].noclip = false;

	// NPC
	gameState.characters[1] = (Character){ 0 }; 
	gameState.characters[1].size = (Vector3){ 0.90f, 1.90f, 0.90f };
	gameState.characters[1].position = (Vector3){ 5.0f, 0.0f, 0.0f };
	gameState.characters[1].isHuman = false;
	gameState.characters[1].isPlayer = false;
	gameState.characters[1].speed = PLAYER_SPEED;
	gameState.characters[1].jumpForce = PLAYER_JUMP_FORCE;
	gameState.characters[1].yaw = 3.14f;
	gameState.characters[1].pitch = 0.0f;
	gameState.characters[1].model = gameState.playerModel;
	gameState.characters[1].footstepTimer = 0.0f;
	gameState.characters[1].jumpBoost = false;
	gameState.characters[1].landFlag = false;
	for (int i = 0; i < NUM_INVENTORY_ITEMS; i++) gameState.characters[1].inventory[i] = false;
	gameState.characters[1].inventoryIndex = 0;
	gameState.characters[1].health = 100.0f;
	gameState.characters[1].color = BLUE;
	gameState.characters[1].noclip = false;

	// Create player weapons
	gameState.playerTeSpecial = (TeSpecial){ 6, 6, 144, 144, 0.0f, -1, CreateLight(LIGHT_POINT, Vector3Zero(), Vector3Zero(), (Color){ 255, 214, 100, 255 }, 0.1f, gameState.basicLightingShader) };
	gameState.playerTeSpecial.flareLight.enabled = false;
	bool showTeSpecialHud = false;

	gameState.playerEditTool = (EditTool){ MOVE, false };

	// Create boxes
	Model boxModel = LoadModelFromMesh(GenMeshCube(BOX_SIZE, BOX_SIZE, BOX_SIZE));
	for (int i = 0; i < boxModel.materialCount; i++) boxModel.materials[i].shader = gameState.basicLightingShader;
	gameState.lenBoxes = 1;
	gameState.boxes[0] = (Box){(Vector3){ 1.0f, 0.5f, 4.0f }, (Vector3){ BOX_SIZE, BOX_SIZE, BOX_SIZE }, boxModel, ORANGE, 100.0f};

	// Create platforms
	gameState.unitCubeModel = LoadModelFromMesh(GenMeshCube(BOX_SIZE, BOX_SIZE, BOX_SIZE));
	for (int i = 0; i < gameState.unitCubeModel.materialCount; i++) gameState.unitCubeModel.materials[i].shader = gameState.basicLightingShader;
	gameState.lenPlatforms = 5;
	gameState.platforms[0] = (Platform){ .position = (Vector3){ 0.0f, -0.25f, 0.0f }, .size = (Vector3){ 20.0f, 0.5f, 20.0f }, .color = GREEN, .solid = true, .model = gameState.unitCubeModel };
	gameState.platforms[0].scale = gameState.platforms[0].size;
	gameState.platforms[1] = (Platform){ .position = (Vector3){ 10.25f, 1.0f, 0.0f }, .size = (Vector3){ 0.5f, 2.0f, 20.0f }, .color = GRAY, .solid = true, .model = gameState.unitCubeModel };
	gameState.platforms[1].scale = gameState.platforms[1].size;
	gameState.platforms[2] = (Platform){ .position = (Vector3){ -10.25f, 1.0f, 0.0f }, .size = (Vector3){ 0.5f, 2.0f, 20.0f }, .color = GRAY, .solid = true, .model = gameState.unitCubeModel };
	gameState.platforms[2].scale = gameState.platforms[2].size;
	gameState.platforms[3] = (Platform){ .position = (Vector3){ 0.0f, 1.0f, 10.25f }, .size = (Vector3){ 20.0f, 2.0f, 0.5f }, .color = GRAY, .solid = true, .model = gameState.unitCubeModel };
	gameState.platforms[3].scale = gameState.platforms[3].size;
	gameState.platforms[4] = (Platform){ .position = (Vector3){ 0.0f, 1.0f, -10.25f }, .size = (Vector3){ 20.0f, 2.0f, 0.5f }, .color = GRAY, .solid = true, .model = gameState.unitCubeModel };
	gameState.platforms[4].scale = gameState.platforms[4].size;

	// Create pickups
	gameState.lenPickups = 6;
	gameState.pickups[0] = (Pickup){ .type = TESPECIAL_AMMO, .canRespawn = true, .respawnTime = 40.0f, .respawnTimer = 0.0f, .enabled = true, .position = (Vector3){ 2.0f, 0.95f, 4.0f }, .size = (Vector3){ 0.5f, 0.5f, 0.5f }, .model = gameState.teAmmoModel, .spin = 0.0f, .spinSpeed = 1.0f, .targetY = 0.95f, .bounceUp = true, .bounceTime = 0.0f, .bounceDuration = 1.0f };
	gameState.pickups[0].light = CreateLight(LIGHT_POINT, gameState.pickups[0].position, Vector3Zero(), (Color){ 255, 214, 100, 255 }, 0.3f, gameState.basicLightingShader);
	gameState.pickups[1] = (Pickup){ .type = TESPECIAL_AMMO, .canRespawn = true, .respawnTime = 40.0f, .respawnTimer = 0.0f, .enabled = true, .position = (Vector3){ -3.0f, 3.95f, 4.0f }, .size = (Vector3){ 0.5f, 0.5f, 0.5f }, .model = gameState.teAmmoModel, .spin = 0.0f, .spinSpeed = 1.0f, .targetY = 3.95f, .bounceUp = true, .bounceTime = 0.0f, .bounceDuration = 1.0f };
	gameState.pickups[1].light = CreateLight(LIGHT_POINT, gameState.pickups[1].position, Vector3Zero(), (Color){ 255, 214, 100, 255 }, 0.3f, gameState.basicLightingShader);
	gameState.pickups[2] = (Pickup){ .type = TESPECIAL_AMMO, .canRespawn = true, .respawnTime = 40.0f, .respawnTimer = 0.0f, .enabled = true, .position = (Vector3){ 2.0f, 0.95f, -5.0f }, .size = (Vector3){ 0.5f, 0.5f, 0.5f }, .model = gameState.teAmmoModel, .spin = 0.0f, .spinSpeed = 1.0f, .targetY = 0.95f, .bounceUp = true, .bounceTime = 0.0f, .bounceDuration = 1.0f };
	gameState.pickups[2].light = CreateLight(LIGHT_POINT, gameState.pickups[2].position, Vector3Zero(), (Color){ 255, 214, 100, 255 }, 0.3f, gameState.basicLightingShader);
	gameState.pickups[3] = (Pickup){ .type = DONUT, .canRespawn = true, .respawnTime = 40.0f, .respawnTimer = 0.0f, .enabled = true, .position = (Vector3){ -5.0f, 0.95f, -5.0f }, .size = (Vector3){ 0.5f, 0.5f, 0.5f }, .model = gameState.donutModel, .spin = 0.0f, .spinSpeed = 1.0f, .targetY = 0.95f, .bounceUp = true, .bounceTime = 0.0f, .bounceDuration = 1.0f };
	gameState.pickups[3].light = CreateLight(LIGHT_POINT, gameState.pickups[3].position, Vector3Zero(), (Color){ 255, 214, 100, 255 }, 0.3f, gameState.basicLightingShader);
	gameState.pickups[4] = (Pickup){ .type = DONUT, .canRespawn = true, .respawnTime = 40.0f, .respawnTimer = 0.0f, .enabled = true, .position = (Vector3){ -10.0f, 0.95f, 9.0f }, .size = (Vector3){ 0.5f, 0.5f, 0.5f }, .model = gameState.donutModel, .spin = 0.0f, .spinSpeed = 1.0f, .targetY = 0.95f, .bounceUp = true, .bounceTime = 0.0f, .bounceDuration = 1.0f };
	gameState.pickups[4].light = CreateLight(LIGHT_POINT, gameState.pickups[4].position, Vector3Zero(), (Color){ 255, 214, 100, 255 }, 0.3f, gameState.basicLightingShader);
	gameState.pickups[5] = (Pickup){ .type = DONUT, .canRespawn = true, .respawnTime = 40.0f, .respawnTimer = 0.0f, .enabled = true, .position = (Vector3){ -1.0f, 0.95f, 45.0f }, .size = (Vector3){ 0.5f, 0.5f, 0.5f }, .model = gameState.donutModel, .spin = 0.0f, .spinSpeed = 1.0f, .targetY = 0.95f, .bounceUp = true, .bounceTime = 0.0f, .bounceDuration = 1.0f };
	gameState.pickups[5].light = CreateLight(LIGHT_POINT, gameState.pickups[5].position, Vector3Zero(), (Color){ 255, 214, 100, 255 }, 0.3f, gameState.basicLightingShader);

	gameState.gravity = 10.0f;
	float mouseSensitivity = 0.003f;

	RayCollision clickCollision = { 0 };
	Ray clickRay = { 0 };

	SetTargetFPS(60);

	// Main game loop
	while (!WindowShouldClose() && !exitWindow) {
		// Update
		// ------------------------------------------

		// Basic fullscreen toggle
		if (IsKeyPressed(KEY_F11)) {
			ToggleFullscreen();
			screenWidth = GetScreenWidth();
			screenHeight = GetScreenHeight();
		}

		// Toggle console
		if (IsKeyPressed(KEY_GRAVE)) {
			console.visible = !console.visible;
		}

		if (console.visible && console.enabled) {
			UpdateConsole(&console, &gameState);
			gameState.characters[0].isHuman = false;
		} else gameState.characters[0].isHuman = true;

		switch (screen) {
			case MENU: {
				switch (menuScreen) {
					case MAIN_MENU: {
						// Menu buttons
						newGameButton.position = (Vector2){ (screenWidth / 2) - (newGameButton.size.x / 2), (screenHeight / 2) - (newGameButton.size.y / 2) - 25 };
						UpdateMenuButton(&newGameButton, fxUIHover, fxUIClick);
						if (newGameButton.clicked) {
							DisableCursor();
							screen = GAMEPLAY;
							newGameButton.clicked = false;
						}

						loadGameButton.position = (Vector2){ (screenWidth / 2) - (loadGameButton.size.x / 2), (screenHeight / 2) - (loadGameButton.size.y / 2) + 50 };
						UpdateMenuButton(&loadGameButton, fxUIHover, fxUIClick);
						if (loadGameButton.clicked) {
							menuScreen = LOAD_GAME;
							loadGameButton.clicked = false;
						}

						quitGameButton.position = (Vector2){ (screenWidth / 2) - (quitGameButton.size.x / 2), (screenHeight / 2) - (quitGameButton.size.y / 2) + 125 };
						UpdateMenuButton(&quitGameButton, fxUIHover, fxUIClick);
						if (quitGameButton.clicked) {
							exitWindow = true;
							quitGameButton.clicked = false;
						}
					} break;

					case LOAD_GAME: {
						// Buttons
						returnToMainButton.position = (Vector2){ 10, screenHeight - (returnToMainButton.size.y + 10) };
						UpdateMenuButton(&returnToMainButton, fxUIHover, fxUIClick);
						if (returnToMainButton.clicked) {
							menuScreen = MAIN_MENU;
							returnToMainButton.clicked = false;
						}

						// File buttons
						int yOffset = 0;
						for (int i = 0; i < (int)saveFiles.count; i++) {
							fileButtons[i].position = (Vector2){ (screenWidth / 2) - (fileButtons[i].size.x / 2), (screenHeight / 2 - 90) + yOffset };
							UpdateMenuButton(&fileButtons[i], fxUIHover, fxUIClick);
							if (fileButtons[i].clicked) {
								if (LoadGameData(saveFiles.paths[i], &gameState.characters[0], gameState.boxes, &gameState.lenBoxes, boxModel)) {
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

					for (int i = 0; i < gameState.lenCharacters; i++) {
						if (gameState.characters[i].isHuman) {
							UpdatePlayerCharacter(&gameState.characters[i], &gameState.playerCamera, mouseSensitivity, gameState.gravity, gameState.boxes, gameState.lenBoxes, gameState.platforms, gameState.lenPlatforms, gameState.pickups, gameState.lenPickups, &gameState.playerTeSpecial, fxStep, fxJump, fxLand, &getAmmoPool, gameState.basicLightingShader);
							UpdateCharacterInventory(&gameState.characters[i], &showInventory, &showInventoryTimer, gameState.boxes, &gameState.lenBoxes, &clickRay, clickCollision, &selectedColorY, &selectedColorX, &showColorPicker, &colorPickerTimer, colors, colorsPerRow, colorsPerCol, gameState.playerCamera, boxModel, fxHitBox, fxBreakBox, fxPlaceBox, fxChangeColor, &gameState.playerTeSpecial, &teShotPool, &reloadPool, &triggerPullPool, &showTeSpecialHud, gameState.platforms, &gameState.lenPlatforms, &gameState.playerEditTool);
						}
						else {
							UpdateNPC(&gameState.characters[i], gameState.playerCamera, gameState.gravity, gameState.boxes, gameState.lenBoxes, gameState.platforms, gameState.lenPlatforms, fxStep, fxLand);
						}
					}

					// Update some lights
					UpdateLightValues(gameState.basicLightingShader, gameState.playerTeSpecial.flareLight);

					// Update pickups
					UpdatePickups(gameState.pickups, gameState.lenPickups, gameState.playerCamera, &pickupPulsePool, &pickupRespawnPool, gameState.basicLightingShader);
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
							resumeButton.position = (Vector2){ (screenWidth / 2) - (resumeButton.size.x / 2), (screenHeight / 2) - (resumeButton.size.y / 2) - 25 };
							UpdateMenuButton(&resumeButton, fxUIHover, fxUIClick);
							if (resumeButton.clicked) {
								DisableCursor();
								gamePaused = false;
								resumeButton.clicked = false;
							}

							saveGameButton.position = (Vector2){ (screenWidth / 2) - (saveGameButton.size.x / 2), (screenHeight / 2) - (saveGameButton.size.y / 2) + 50 };
							UpdateMenuButton(&saveGameButton, fxUIHover, fxUIClick);
							if (saveGameButton.clicked) {
								pauseScreen = SAVE_GAME;
								saveGameButton.clicked = false;
							}

							quitToMenuButton.position = (Vector2){ (screenWidth / 2) - (quitToMenuButton.size.x / 2), (screenHeight / 2) - (quitToMenuButton.size.y / 2) + 125 };
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
							returnToMainButton.position = (Vector2){ 10, screenHeight - (returnToMainButton.size.y + 10) };
							UpdateMenuButton(&returnToMainButton, fxUIHover, fxUIClick);
							if (returnToMainButton.clicked) {
								pauseScreen = MAIN_PAUSE_MENU;
								returnToMainButton.clicked = false;
							}

							// New save button
							int yOffset = 0;
							newSaveButton.position = (Vector2){ (screenWidth / 2) - (newSaveButton.size.x / 2), (screenHeight / 2 - 90) + yOffset };
							UpdateMenuButton(&newSaveButton, fxUIHover, fxUIClick);
							if (newSaveButton.clicked) {
								char *newSaveName = GetNewSaveFileName(&saveFiles);

								if (newSaveName != NULL) {
									if (SaveGameData(newSaveName, &gameState.characters[0], gameState.boxes, gameState.lenBoxes)) {
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
								fileButtons[i].position = (Vector2){ (screenWidth / 2) - (fileButtons[i].size.x / 2), (screenHeight / 2 - 90) + yOffset };
								UpdateMenuButton(&fileButtons[i], fxUIHover, fxUIClick);
								if (fileButtons[i].clicked) {
									if (SaveGameData(saveFiles.paths[i], &gameState.characters[0], gameState.boxes, gameState.lenBoxes)) {
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
				float playerCameraPos[3] = { gameState.playerCamera.position.x, gameState.playerCamera.position.y, gameState.playerCamera.position.z };
				SetShaderValue(gameState.basicLightingShader, gameState.basicLightingShader.locs[SHADER_LOC_VECTOR_VIEW], playerCameraPos, SHADER_UNIFORM_VEC3);

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
							DrawText("Josh's Raylib Sandbox", screenWidth/2 - MeasureText("Josh's Raylib Sandbox", 50)/2, screenHeight/2 - 150, 50, RAYWHITE);

							// Draw menu buttons
							DrawMenuButton(newGameButton);
							DrawMenuButton(loadGameButton);
							DrawMenuButton(quitGameButton);

							// Draw console
							if (console.enabled && console.visible) DrawConsole(console, screenWidth, screenHeight);
						EndDrawing();
					} break;

					case LOAD_GAME: {
						BeginDrawing();
							ClearBackground(BLACK);

							// Draw page title
							DrawText("Load Game", screenWidth/2 - MeasureText("Load Game", 50)/2, screenHeight/2 - 150, 50, RAYWHITE);

							// Draw files list buttons
							for (int i = 0; i < (int)saveFiles.count; i++) {
								DrawMenuButton(fileButtons[i]);
							}

							DrawMenuButton(returnToMainButton);

							// Draw console
							if (console.enabled && console.visible) DrawConsole(console, screenWidth, screenHeight);
						EndDrawing();
					} break;

					default: break;
				}
			} break;

			case GAMEPLAY: {
				// Construct viewport.
				BeginTextureMode(viewport);
					ClearBackground(BLACK);

					BeginMode3D(gameState.genericCamera);

						BeginShaderMode(gameState.basicLightingShader);

							// Draw characters 
							for (int i = 0; i < gameState.lenCharacters; i++) {
								DrawModelEx(gameState.characters[i].model, gameState.characters[i].position, (Vector3){ 0.0f, 1.0f, 0.0f }, -(gameState.characters[i].yaw * 180.0f / PI), (Vector3){ 1.1f, 1.1f, 1.1f }, gameState.characters[i].color);
								DrawCubeWiresV(gameState.characters[i].position, gameState.characters[i].size, GREEN);
							}

							// Draw player inventory item
							switch (gameState.characters[0].inventoryIndex) {
								case INVENTORY_NOTHING: {
									break;
								}; break;
								case INVENTORY_BUILD_TOOL: {
									DrawItemBuildTool(gameState.playerCamera, boxModel, colors, selectedColorX, selectedColorY);
								}; break;
								case INVENTORY_TESPECIAL: {
									DrawItemTeSpecial(&gameState.playerTeSpecial, gameState.playerCamera, teSpecialModel, bigMuzzleFlareModel, littleMuzzleFlareModel);
								}; break;
								default: break;
							}

							// Draw boxes
							DrawBoxes(gameState.boxes, gameState.lenBoxes);

							// Draw platforms
							DrawPlatforms(gameState.platforms, gameState.lenPlatforms);

							// Draw pickups
							DrawPickups(gameState.pickups, gameState.lenPickups);

						EndShaderMode();

						// Draw floor
						DrawFloor(20, 20, 1.0f, -0.5f * 20.0f * 1.0f - 0.5f * 1.0f, -0.5f * 20.0f * 1.0f - 0.5f * 1.0f);

						// Draw clickRay
						DrawRay(clickRay, RED);

						// Draw platform selection indicator lines
						if (gameState.playerEditTool.showSelection) DrawPlatformSelection(gameState.platforms, gameState.lenPlatforms);

					EndMode3D();
				EndTextureMode();

				BeginDrawing();
					ClearBackground(BLACK);

					BeginMode3D(gameState.playerCamera);

						BeginShaderMode(gameState.basicLightingShader);
	
							// Draw boxes
							DrawBoxes(gameState.boxes, gameState.lenBoxes);

							// Draw platforms
							DrawPlatforms(gameState.platforms, gameState.lenPlatforms);
	
							// Draw pickups
							DrawPickups(gameState.pickups, gameState.lenPickups);

							// Draw player inventory item
							switch (gameState.characters[0].inventoryIndex) {
								case INVENTORY_NOTHING: {
									break;
								}; break;
								case INVENTORY_BUILD_TOOL: {
									DrawItemBuildTool(gameState.playerCamera, boxModel, colors, selectedColorX, selectedColorY);
								}; break;
								case INVENTORY_TESPECIAL: {
									DrawItemTeSpecial(&gameState.playerTeSpecial, gameState.playerCamera, teSpecialModel, bigMuzzleFlareModel, littleMuzzleFlareModel);
								}
								default: break;
							}

							// Draw characters (Except for player.)
							for (int i = 0; i < gameState.lenCharacters; i++) {
								if (!gameState.characters[i].isHuman) {
									DrawModelEx(gameState.characters[i].model, gameState.characters[i].position, (Vector3){ 0.0f, 1.0f, 0.0f }, -(gameState.characters[i].yaw * 180.0f / PI), (Vector3){ 1.1f, 1.1f, 1.1f }, gameState.characters[i].color);
									DrawCubeWiresV(gameState.characters[i].position, gameState.characters[i].size, GREEN);
								}
							}

						EndShaderMode();

						// Draw floor 
						//DrawFloor(20, 20, 1.0f, -0.5f * 20.0f * 1.0f - 0.5f * 1.0f, -0.5f * 20.0f * 1.0f - 0.5f * 1.0f);

						// Draw clickRay
						DrawRay(clickRay, RED);

						// Draw platform selection indicator lines
						if (gameState.playerEditTool.showSelection) DrawPlatformSelection(gameState.platforms, gameState.lenPlatforms);

					EndMode3D();

					// Draw crosshair
					DrawRectangle((screenWidth / 2) - (8 / 2), (screenHeight / 2) - (2 / 2), 8, 2, DARKGREEN);
					DrawRectangle((screenWidth / 2) - (2 / 2), (screenHeight / 2) - (8 / 2), 2, 8, DARKGREEN);

					// Draw viewport
					DrawTextureRec(viewport.texture, viewportRect, (Vector2){ 10, 10 }, WHITE);

					// Draw fps
					DrawFPS(screenWidth - 100, 10);

					// Color picker
					if (showColorPicker) {
						DrawColorPicker(colorsPerCol, colorsPerRow, selectedColorX, selectedColorY, colors);
					}

					// Inventory
					if (showInventory) {
						// Evaluate current item
						const char* itemText = TextFormat("%d", gameState.characters[0].inventoryIndex);
						switch (gameState.characters[0].inventoryIndex) {
							case INVENTORY_NOTHING: itemText = "None"; break;
							case INVENTORY_EDIT_TOOL: itemText = "Edit Tool"; break;
							case INVENTORY_BUILD_TOOL: itemText = "Build Tool"; break;
							case INVENTORY_TESPECIAL: itemText = ".38 Special"; break;
							default: break;
						}

						DrawText(itemText, screenWidth/2 - MeasureText(itemText, 50)/2, screenHeight/2 - 150, 50, RAYWHITE);
					}

					// .38 Special ammunition
					if (showTeSpecialHud) {
						const char* ammoText = TextFormat("%d/%d", gameState.playerTeSpecial.roundsLoaded, gameState.playerTeSpecial.extraRounds);
						DrawText(ammoText, screenWidth - MeasureText(ammoText, 50) - (screenWidth / 100), screenHeight - 50, 50, RAYWHITE);
					}

					// Game Paused
					if (gamePaused) {
						// Draw pause backdrop
						DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.5f));

						switch (pauseScreen) {
							case MAIN_PAUSE_MENU: {
								DrawText("Paused", screenWidth/2 - MeasureText("Paused", 50)/2, screenHeight/2 - 150, 50, RAYWHITE);

								// Draw buttons
								DrawMenuButton(resumeButton);
								DrawMenuButton(saveGameButton);
								DrawMenuButton(quitToMenuButton);
							} break;

							case SAVE_GAME: {
								DrawText("Save Game", screenWidth/2 - MeasureText("Save Game", 50)/2, screenHeight/2 - 150, 50, RAYWHITE);

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

					// Draw console
					if (console.enabled && console.visible) DrawConsole(console, screenWidth, screenHeight);
				EndDrawing();
			} break;

			default: break;
		}
		// ------------------------------------------
	}

	// De-initialization
	UnloadShader(gameState.basicLightingShader);

	UnloadModel(gameState.playerModel);
	UnloadModel(boxModel);
	UnloadModel(gameState.donutModel);
	UnloadModel(teSpecialModel);
	UnloadModel(littleMuzzleFlareModel);
	UnloadModel(bigMuzzleFlareModel);
	UnloadModel(gameState.teAmmoModel);
	UnloadModel(gameState.unitCubeModel);

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
