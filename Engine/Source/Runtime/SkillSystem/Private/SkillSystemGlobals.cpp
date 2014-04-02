// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "SkillSystemModulePrivatePCH.h"

USkillSystemGlobals::USkillSystemGlobals(const class FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{

}

UCurveTable * USkillSystemGlobals::GetGlobalCurveTable()
{
	if (!GlobalCurveTable && !GlobalCurveTableName.IsEmpty())
	{
		GlobalCurveTable = LoadObject<UCurveTable>(NULL, *GlobalCurveTableName, NULL, LOAD_None, NULL);
#if WITH_EDITOR
		// Hook into notifications for object re-imports so that the gameplay tag tree can be reconstructed if the table changes
		if (GIsEditor && GlobalCurveTable)
		{
			GEditor->OnObjectReimported().AddUObject(this, &USkillSystemGlobals::OnObjectReimported);
		}
#endif
	}
	
	return GlobalCurveTable;
}

#if WITH_EDITOR

void USkillSystemGlobals::OnObjectReimported(UObject* InObject)
{
	if (GIsEditor && !IsRunningCommandlet() && InObject && InObject == GlobalCurveTable)
	{
	}
}

#endif