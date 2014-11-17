// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

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
			GEditor->OnObjectReimported().AddUObject(this, &USkillSystemGlobals::OnCurveTableReimported);
		}
#endif
	}
	
	return GlobalCurveTable;
}

UDataTable * USkillSystemGlobals::GetGlobalAttributeDataTable()
{
	if (!GlobalAttributeDataTable && !GlobalAttributeDataTableName.IsEmpty())
	{
		GlobalAttributeDataTable = LoadObject<UDataTable>(NULL, *GlobalAttributeDataTableName, NULL, LOAD_None, NULL);
#if WITH_EDITOR
		// Hook into notifications for object re-imports so that the gameplay tag tree can be reconstructed if the table changes
		if (GIsEditor && GlobalAttributeDataTable)
		{
			GEditor->OnObjectReimported().AddUObject(this, &USkillSystemGlobals::OnDataTableReimported);
		}
#endif
	}

	return GlobalAttributeDataTable;
}

#if WITH_EDITOR

void USkillSystemGlobals::OnCurveTableReimported(UObject* InObject)
{
	if (GIsEditor && !IsRunningCommandlet() && InObject && InObject == GlobalCurveTable)
	{
	}
}

void USkillSystemGlobals::OnDataTableReimported(UObject* InObject)
{
	if (GIsEditor && !IsRunningCommandlet() && InObject && InObject == GlobalAttributeDataTable)
	{
	}
}

#endif
