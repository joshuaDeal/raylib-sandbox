#include "character.h"
#include <cjson/cJSON.h>
#include <time.h>
#include "buildtool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool SaveGameData(const char *filePath, const Character *player, const Box *boxes, int lenBoxes) {
	// Create json root
	cJSON *root = cJSON_CreateObject();

	// If we couldn't create the root, return false
	if (root == NULL) return false;

	// Meta-data about the save file
	cJSON_AddNumberToObject(root, "version", 1);
	cJSON_AddNumberToObject(root, "timestamp", time(NULL));

	// Player
	cJSON *playerJson = cJSON_AddObjectToObject(root, "player");

	cJSON *positionJson = cJSON_AddObjectToObject(playerJson, "position");
	cJSON_AddNumberToObject(positionJson, "x", player->position.x);
	cJSON_AddNumberToObject(positionJson, "y", player->position.y);
	cJSON_AddNumberToObject(positionJson, "z", player->position.z);

	cJSON *velocityJson = cJSON_AddObjectToObject(playerJson, "velocity");
	cJSON_AddNumberToObject(velocityJson, "x", player->velocity.x);
	cJSON_AddNumberToObject(velocityJson, "y", player->velocity.y);
	cJSON_AddNumberToObject(velocityJson, "z", player->velocity.z);

	cJSON_AddNumberToObject(playerJson, "yaw", player->yaw);
	cJSON_AddNumberToObject(playerJson, "pitch", player->pitch);
	cJSON_AddBoolToObject(playerJson, "isHuman", player->isHuman);

	// Boxes
	cJSON *boxesJson = cJSON_AddArrayToObject(root, "boxes");

	for (int i = 0; i < lenBoxes; i++) {
		cJSON *boxJson = cJSON_CreateObject();

		cJSON *boxPositionJson = cJSON_AddObjectToObject(boxJson, "position");

		cJSON_AddNumberToObject(boxPositionJson, "x", boxes[i].position.x);
		cJSON_AddNumberToObject(boxPositionJson, "y", boxes[i].position.y);
		cJSON_AddNumberToObject(boxPositionJson, "z", boxes[i].position.z);

		cJSON *colorJson = cJSON_AddObjectToObject(boxJson, "color");

		cJSON_AddNumberToObject(colorJson, "r", boxes[i].color.r);
		cJSON_AddNumberToObject(colorJson, "g", boxes[i].color.g);
		cJSON_AddNumberToObject(colorJson, "b", boxes[i].color.b);
		cJSON_AddNumberToObject(colorJson, "a", boxes[i].color.a);

		cJSON_AddNumberToObject(boxJson, "health", boxes[i].health);

		cJSON_AddItemToArray(boxesJson, boxJson);
	}

	// Convert the JSON tree into a formatted string
	char *jsonString = cJSON_Print(root);

	// Return false if we don't have a jsonString
	if (jsonString == NULL) {
		cJSON_Delete(root);
		return false;
	}

	// Opening with "w" overwrites the file if it already exists.
	FILE *file = fopen(filePath, "w");

	if (file == NULL) {
		cJSON_free(jsonString);
		cJSON_Delete(root);
		return false;
	}

	if (fputs(jsonString, file) == EOF) {
		fclose(file);
		cJSON_free(jsonString);
		cJSON_Delete(root);
		return false;
	}

	fclose(file);

	cJSON_free(jsonString);
	cJSON_Delete(root);

	return true;
}

bool LoadGameData(const char *filePath, Character *player, Box *boxes, int *lenBoxes, Model boxModel) {
	Box tmpBoxes[MAX_BOXES];

	// Read the entire file into memory.
	FILE *file = fopen(filePath, "r");

	if (file == NULL) return false;

	// Put the file position indicator at end of file.
	fseek(file, 0, SEEK_END);
	// ftell() gives us the current file position indicator. Since it's at the end of the file, this gives us the file size.
	long fileSize = ftell(file);
	// Move file position indicator back to the beginning of the file.
	rewind(file);

	// If ftell had failed, fileSize would be negative.
	if (fileSize < 0) {
		fclose(file);
		return false;
	}

	// Allocate enough memory for every byte in the file, plus one byte for the terminating null character
	char *jsonString = malloc(fileSize + 1);

	if (jsonString == NULL) {
		fclose(file);
		return false;
	}

	// Populates jsonString with data from file.
	size_t bytesRead = fread(jsonString, 1, fileSize, file);

	fclose(file);

	// bytesRead being the wrong size would mean that something went wrong with reading the file.
	if (bytesRead != (size_t)fileSize) {
		free(jsonString);
		return false;
	}

	jsonString[fileSize] = '\0';

	// Parse the JSON string.
	cJSON *root = cJSON_Parse(jsonString);

	free(jsonString);

	if (root == NULL) return false;

	// Version
	cJSON *versionJson = cJSON_GetObjectItemCaseSensitive(root, "version");

	if (!cJSON_IsNumber(versionJson) || versionJson->valueint != 1) {
		cJSON_Delete(root);
		return false;
	}

	// Player
	cJSON *playerJson = cJSON_GetObjectItemCaseSensitive(root, "player");

	if (!cJSON_IsObject(playerJson)) {
		cJSON_Delete(root);
		return false;
	}

	cJSON *positionJson = cJSON_GetObjectItemCaseSensitive(playerJson, "position");
	cJSON *velocityJson = cJSON_GetObjectItemCaseSensitive(playerJson, "velocity");
	cJSON *yawJson = cJSON_GetObjectItemCaseSensitive(playerJson, "yaw");
	cJSON *pitchJson = cJSON_GetObjectItemCaseSensitive(playerJson, "pitch");
	cJSON *isHumanJson = cJSON_GetObjectItemCaseSensitive(playerJson, "isHuman");

	if (!cJSON_IsObject(positionJson) || !cJSON_IsObject(velocityJson) || !cJSON_IsNumber(yawJson) || !cJSON_IsNumber(pitchJson) || !cJSON_IsBool(isHumanJson)) {
		cJSON_Delete(root);
		return false;
	}

	cJSON *playerPositionX = cJSON_GetObjectItemCaseSensitive(positionJson, "x");
	cJSON *playerPositionY = cJSON_GetObjectItemCaseSensitive(positionJson, "y");
	cJSON *playerPositionZ = cJSON_GetObjectItemCaseSensitive(positionJson, "z");
	cJSON *velocityX = cJSON_GetObjectItemCaseSensitive(velocityJson, "x");
	cJSON *velocityY = cJSON_GetObjectItemCaseSensitive(velocityJson, "y");
	cJSON *velocityZ = cJSON_GetObjectItemCaseSensitive(velocityJson, "z");

	if (!cJSON_IsNumber(playerPositionX) || !cJSON_IsNumber(playerPositionY) || !cJSON_IsNumber(playerPositionZ) || !cJSON_IsNumber(velocityX) || !cJSON_IsNumber(velocityY) || !cJSON_IsNumber(velocityZ)) {
		cJSON_Delete(root);
		return false;
	}

	// Boxes
	cJSON *boxesJson = cJSON_GetObjectItemCaseSensitive(root, "boxes");

	if (!cJSON_IsArray(boxesJson)) {
		cJSON_Delete(root);
		return false;
	}

	int boxCount = cJSON_GetArraySize(boxesJson);

	if (boxCount > MAX_BOXES) {
		cJSON_Delete(root);
		return false;
	}

	for (int i = 0; i < boxCount; i++) {
		cJSON *boxJson = cJSON_GetArrayItem(boxesJson, i);

		if (!cJSON_IsObject(boxJson)) {
			cJSON_Delete(root);
			return false;
		}

		cJSON *positionJson = cJSON_GetObjectItemCaseSensitive(boxJson, "position");
		cJSON *colorJson = cJSON_GetObjectItemCaseSensitive(boxJson, "color");
		cJSON *healthJson = cJSON_GetObjectItemCaseSensitive(boxJson, "health");

		if (!cJSON_IsObject(positionJson) || !cJSON_IsObject(colorJson) || !cJSON_IsNumber(healthJson)) {
			cJSON_Delete(root);
			return false;
		}

		cJSON *positionX = cJSON_GetObjectItemCaseSensitive(positionJson, "x");
		cJSON *positionY = cJSON_GetObjectItemCaseSensitive(positionJson, "y");
		cJSON *positionZ = cJSON_GetObjectItemCaseSensitive(positionJson, "z");

		cJSON *colorR = cJSON_GetObjectItemCaseSensitive(colorJson, "r");
		cJSON *colorG = cJSON_GetObjectItemCaseSensitive(colorJson, "g");
		cJSON *colorB = cJSON_GetObjectItemCaseSensitive(colorJson, "b");
		cJSON *colorA = cJSON_GetObjectItemCaseSensitive(colorJson, "a");

		if (!cJSON_IsNumber(positionX) || !cJSON_IsNumber(positionY) || !cJSON_IsNumber(positionZ) || !cJSON_IsNumber(colorR) || !cJSON_IsNumber(colorG) || !cJSON_IsNumber(colorB) || !cJSON_IsNumber(colorA)) {
			cJSON_Delete(root);
			return false;
		}

		tmpBoxes[i].position.x = (float)positionX->valuedouble;
		tmpBoxes[i].position.y = (float)positionY->valuedouble;
		tmpBoxes[i].position.z = (float)positionZ->valuedouble;

		tmpBoxes[i].color.r = (unsigned char)colorR->valueint;
		tmpBoxes[i].color.g = (unsigned char)colorG->valueint;
		tmpBoxes[i].color.b = (unsigned char)colorB->valueint;
		tmpBoxes[i].color.a = (unsigned char)colorA->valueint;

		tmpBoxes[i].health = (float)healthJson->valuedouble;
	}

	*lenBoxes = boxCount;

	player->position.x = (float)playerPositionX->valuedouble;
	player->position.y = (float)playerPositionY->valuedouble;
	player->position.z = (float)playerPositionZ->valuedouble;

	player->velocity.x = (float)velocityX->valuedouble;
	player->velocity.y = (float)velocityY->valuedouble;
	player->velocity.z = (float)velocityZ->valuedouble;

	player->yaw = (float)yawJson->valuedouble;
	player->pitch = (float)pitchJson->valuedouble;
	player->isHuman = cJSON_IsTrue(isHumanJson);

	// Copy tmpBoxes to boxes.
	for (int i = 0; i < boxCount; i++) {
		boxes[i] = tmpBoxes[i];
		boxes[i].model = boxModel;
		boxes[i].size = (Vector3){ BOX_SIZE, BOX_SIZE, BOX_SIZE };
	}

	cJSON_Delete(root);

	return true;
}

char *GetSaveDirectory(void) {
	const char *workingDirectory = GetWorkingDirectory();
	const char *suffix = "/save-data";

	size_t length = strlen(workingDirectory) + strlen(suffix) + 1;
	char *saveDirectory = malloc(length);

	if (saveDirectory == NULL) {
		return NULL;
	}

	snprintf(saveDirectory, length, "%s%s", workingDirectory, suffix);

	return saveDirectory;
}

char *GetFileNameFromPath(const char *path) {
	const char *filename;
	const char *end;
	size_t length;
	char *result;

	if (path == NULL) {
		return NULL;
	}

	// Ignore trailing slashes, so "/path/to/file.txt/" gives "file.txt".
	end = path + strlen(path);
	while (end > path && end[-1] == '/') {
		--end;
	}

	// Find the character after the final slash.
	filename = end;
	while (filename > path && filename[-1] != '/') {
		--filename;
	}

	length = (size_t)(end - filename);

	result = malloc(length + 1);
	if (result == NULL) {
		return NULL;
	}

	memcpy(result, filename, length);
	result[length] = '\0';

	return result;
}

char *GetNewSaveFileName(const FilePathList *fileList) {
	bool used[1000] = { false };
	const char *directory = NULL;
	size_t lenDirectory = 0;

	if (fileList == NULL) {
		return NULL;
	}

	for (unsigned int i = 0; i < fileList->count; ++i) {
		const char *path = fileList->paths[i];

		if (path == NULL) {
			continue;
		}

		// Find the final path component.
		const char *filename = strrchr(path, '/');
		filename = filename ? filename + 1 : path;

		int saveNumber;

		// The %n conversion stores how many characters were consumed. This lets us verify that the filename contains exactly "save + three digits + .json"
		int consumed = 0;

		if (sscanf(filename, "save%d.json%n", &saveNumber, &consumed) != 1) {
			continue;
		}

		if (filename[consumed] != '\0' || saveNumber < 1 || saveNumber > 999) {
			continue;
		}

		// Verify that the number had exactly three digits. For example, reject save1.json and save0001.json.
		const char *dot = strstr(filename, ".json");
		size_t lenNumber = (size_t)(dot - (filename + 4));

		if (lenNumber != 3) {
			continue;
		}

		// Record the save number as used.
		used[saveNumber] = true;

		// Save the directory prefix from the first valid path.
		if (directory == NULL) {
			const char *lastSlash = strrchr(path, '/');

			if (lastSlash != NULL) {
				directory = path;
				lenDirectory = (size_t)(lastSlash - path) + 1;
			}
			else {
				directory = "";
				lenDirectory = 0;
			}
		}
	}

	// Find the smallest unused number from 001 through 999.
	int nextNumber = 0;

	for (int i = 1; i <= 999; ++i) {
		if (!used[i]) {
			nextNumber = i;
			break;
		}
	}

	// If all numbers from 001 through 999 are already used.
	if (nextNumber == 0) {
		return NULL;
	}

	// If no valid save files were found, use the default save directory.
	char *allocatedDirectory = NULL;
	bool addSlash = false;

	if (directory == NULL) {
		allocatedDirectory = GetSaveDirectory();

		if (allocatedDirectory == NULL) {
			return NULL;
		}

		directory = allocatedDirectory;
		lenDirectory = strlen(directory);

		addSlash = lenDirectory > 0 && directory[lenDirectory - 1] != '/';
	}

	// Allocate space for "directory + "save###.json" + terminating '\0'"
	const char *filenameFormat = "save%03d.json";
	int lenFilename = snprintf(NULL, 0, filenameFormat, nextNumber);

	//char *result = malloc(lenDirectory + (size_t)lenFilename + 1);
	char *result = malloc(lenDirectory + (addSlash ? 1 : 0) + (size_t)lenFilename + 1);

	if (result == NULL) {
		free(allocatedDirectory);
		return NULL;
	}

	memcpy(result, directory, lenDirectory);

	size_t offset = lenDirectory;

	if (addSlash) {
		result[offset++] = '/';
	}

	//snprintf(result + lenDirectory, (size_t)lenFilename + 1, filenameFormat, nextNumber);
	snprintf(result + offset, (size_t)lenFilename + 1, filenameFormat, nextNumber);

	free(allocatedDirectory);

	return result;
}

FilePathList LoadSaveFiles(void) {
	char *saveDirectory = GetSaveDirectory();
	TraceLog(LOG_INFO, "Save directory: %s", saveDirectory);
	FilePathList saveFiles = LoadDirectoryFilesEx(saveDirectory, ".json", false);
	free(saveDirectory);
	return saveFiles;
}
