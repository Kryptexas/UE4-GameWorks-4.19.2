// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SkillSystemModulePrivatePCH.h"
#include "AttributeComponent.h"
#include "SkillSystemTypes.h"

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


UAttributeComponent * USkillSystemGlobals::GetAttributeComponentFromActor(AActor *Actor) const
{
	if (Actor)
	{
		return Actor->FindComponentByClass<UAttributeComponent>();
	}

	// Caller should check this
	ensure(false);
	return NULL;
}

FGameplayAbilityActorInfo * USkillSystemGlobals::AllocAbilityActorInfo(AActor *Actor) const
{
	FGameplayAbilityActorInfo * Info = new FGameplayAbilityActorInfo();
	Info->InitFromActor(Actor);
	return Info;
}

/** This is just some syntax sugar to avoid calling gode to have to IGameplayAbilitiesModule::Get() */
USkillSystemGlobals& USkillSystemGlobals::Get()
{
	static USkillSystemGlobals * GlobalPtr = IGameplayAbilitiesModule::Get().GetSkillSystemGlobals();
	check(GlobalPtr == IGameplayAbilitiesModule::Get().GetSkillSystemGlobals());
	check(GlobalPtr);

	return *GlobalPtr;
}
