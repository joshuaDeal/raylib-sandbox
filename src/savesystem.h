#ifndef SAVESYSTEM_H
#define SAVESYSTEM_H

#include "character.h"
#include "buildtool.h"

bool SaveGameData(const char *filePath, const Character *player, const Box *boxes, int lenBoxes);

bool LoadGameData(const char *filePath, Character *player, Box *boxes, int *lenBoxes, Model boxModel);

char *GetSaveDirectory(void);

char *GetFileNameFromPath(const char *path);

char *GetNewSaveFileName(const FilePathList *fileList);

FilePathList LoadSaveFiles(void);

#endif
