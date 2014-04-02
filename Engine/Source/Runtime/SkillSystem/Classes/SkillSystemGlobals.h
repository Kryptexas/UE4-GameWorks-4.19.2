// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SkillSystemGlobals.generated.h"

/** Holds global data for the skill system. Can be configured per project via config file */
UCLASS(config=Game)
class SKILLSYSTEM_API USkillSystemGlobals : public UObject
{
	GENERATED_UCLASS_BODY()

	/** Holds all of the valid gameplay-related tags that can be applied to assets */
	UPROPERTY(config)
	FString GlobalCurveTableName;

	class UCurveTable *	GetGlobalCurveTable();

	void AutomationTestOnly_SetGlobalCurveTable(class UCurveTable *InTable)
	{
		GlobalCurveTable = InTable;
	}

private:

	class UCurveTable* GlobalCurveTable;

#if WITH_EDITOR
	void OnObjectReimported(UObject* InObject);
#endif

};
