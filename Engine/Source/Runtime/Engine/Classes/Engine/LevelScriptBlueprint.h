// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "LevelScriptBlueprint.generated.h"

UCLASS(dependson=UBlueprint,MinimalAPI, NotBlueprintType)
class ULevelScriptBlueprint : public UBlueprint
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITORONLY_DATA
	/** The friendly name to use for UI */
	UPROPERTY(transient)
	FString FriendlyName;
#endif

#if WITH_EDITOR
	ULevel* GetLevel() const
	{
		return Cast<ULevel>(GetOuter());
	}

	// Begin UBlueprint interface
	ENGINE_API virtual UObject* GetObjectBeingDebugged() OVERRIDE;
	ENGINE_API virtual void SetObjectBeingDebugged(UObject* NewObject) OVERRIDE;
	ENGINE_API virtual FString GetFriendlyName() const OVERRIDE;
	// End UBlueprint interface

	/** Generate a name for a level script blueprint from the current level */
	static FString CreateLevelScriptNameFromLevel (const class ULevel* Level );

#endif	//#if WITH_EDITOR
};
