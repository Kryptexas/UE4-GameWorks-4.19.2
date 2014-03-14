#include "Core.h"
#include "ModuleManager.h"

// Implement the modules
IMPLEMENT_MODULE(FDefaultModuleImpl, UE4Game);

// Variables referenced from Core, and usually declared by IMPLEMENT_PRIMARY_GAME_MODULE. Since UE4Game is game-agnostic, implement them here.
#if IS_MONOLITHIC
PER_MODULE_BOILERPLATE
bool GIsGameAgnosticExe = true;
TCHAR GGameName[64] = TEXT("");
#endif
