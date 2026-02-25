#ifndef __SAVEGAMECONTEXT_H__
#define __SAVEGAMECONTEXT_H__

#include <string>

class Board;
class ISaveProvider;

bool				LawnLoadGame(Board* theBoard, const std::string& theFilePath, ISaveProvider& theSaveProvider);
bool				LawnSaveGame(Board* theBoard, const std::string& theFilePath, ISaveProvider& theSaveProvider);

#endif
