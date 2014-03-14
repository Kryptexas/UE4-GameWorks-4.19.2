#include "CorePrivate.h"
#include "EngineBuildSettings.h"

bool FEngineBuildSettings::IsInternalBuild()
{
	return FPaths::FileExists( FPaths::EngineDir() / TEXT("Build/NotForLicensees/EpicInternal.txt") );
}

bool FEngineBuildSettings::IsPerforceBuild()
{
	return FPaths::FileExists( FPaths::EngineDir() / TEXT("Build/PerforceBuild.txt") );
}