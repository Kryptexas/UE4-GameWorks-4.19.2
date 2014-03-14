// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "BlueprintUtilities.h"
#include "LatentActions.h"
#include "EngineLevelScriptClasses.h"

//////////////////////////////////////////////////////////////////////////
// ULevelScriptBlueprint

ULevelScriptBlueprint::ULevelScriptBlueprint(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

#if WITH_EDITOR

void ULevelScriptBlueprint::SetObjectBeingDebugged(UObject* NewObject)
{
	// Only allowed because UWorld::TransferBlueprintDebugReferences needs to get us looking at our duplicated self
	UBlueprint::SetObjectBeingDebugged(NewObject);
}

UObject* ULevelScriptBlueprint::GetObjectBeingDebugged()
{ 
	UObject* CachedResult = UBlueprint::GetObjectBeingDebugged();

	if (CachedResult != NULL)
	{
		return CachedResult;
	}
	else
	{
		if (ULevel* Level = GetLevel()) 
		{ 
			CachedResult = Level->GetLevelScriptActor(); 
		}

		SetObjectBeingDebugged(CachedResult);
	}

	return CachedResult; 
} 

FString ULevelScriptBlueprint::GetFriendlyName() const
{
#if WITH_EDITORONLY_DATA
	return FriendlyName;
#endif
	return UBlueprint::GetFriendlyName();
}

FString ULevelScriptBlueprint::CreateLevelScriptNameFromLevel (const ULevel* Level)
{
	// Since all maps are named "PersistentLevel," check to see if this level is the actual OwningWorld->PersistentLevel, or name it based on the map package.
	check(Level && Level->OwningWorld);
	UObject* LevelPackage = Level->GetOutermost();

	return FPackageName::GetShortName(LevelPackage->GetFName().GetPlainNameString());

}

#endif	//#if WITH_EDITOR

