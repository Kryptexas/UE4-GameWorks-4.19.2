// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "AbilitySystemComponent.h"
#include "GameplayAbilityTypes.h"

UAbilitySystemGlobals::UAbilitySystemGlobals(const class FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{

}

UCurveTable * UAbilitySystemGlobals::GetGlobalCurveTable()
{
	if (!GlobalCurveTable && !GlobalCurveTableName.IsEmpty())
	{
		GlobalCurveTable = LoadObject<UCurveTable>(NULL, *GlobalCurveTableName, NULL, LOAD_None, NULL);
#if WITH_EDITOR
		// Hook into notifications for object re-imports so that the gameplay tag tree can be reconstructed if the table changes
		if (GIsEditor && GlobalCurveTable)
		{
			GEditor->OnObjectReimported().AddUObject(this, &UAbilitySystemGlobals::OnCurveTableReimported);
		}
#endif
	}
	
	return GlobalCurveTable;
}

UDataTable * UAbilitySystemGlobals::GetGlobalAttributeDataTable()
{
	if (!GlobalAttributeDataTable && !GlobalAttributeDataTableName.IsEmpty())
	{
		GlobalAttributeDataTable = LoadObject<UDataTable>(NULL, *GlobalAttributeDataTableName, NULL, LOAD_None, NULL);
#if WITH_EDITOR
		// Hook into notifications for object re-imports so that the gameplay tag tree can be reconstructed if the table changes
		if (GIsEditor && GlobalAttributeDataTable)
		{
			GEditor->OnObjectReimported().AddUObject(this, &UAbilitySystemGlobals::OnDataTableReimported);
		}
#endif
	}

	return GlobalAttributeDataTable;
}

#if WITH_EDITOR

void UAbilitySystemGlobals::OnCurveTableReimported(UObject* InObject)
{
	if (GIsEditor && !IsRunningCommandlet() && InObject && InObject == GlobalCurveTable)
	{
	}
}

void UAbilitySystemGlobals::OnDataTableReimported(UObject* InObject)
{
	if (GIsEditor && !IsRunningCommandlet() && InObject && InObject == GlobalAttributeDataTable)
	{
	}
}

#endif


UAbilitySystemComponent * UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(AActor *Actor) const
{
	if (Actor)
	{
		return Actor->FindComponentByClass<UAbilitySystemComponent>();
	}

	// Caller should check this
	ensure(false);
	return NULL;
}

FGameplayAbilityActorInfo * UAbilitySystemGlobals::AllocAbilityActorInfo(AActor *Actor) const
{
	FGameplayAbilityActorInfo * Info = new FGameplayAbilityActorInfo();
	Info->InitFromActor(Actor);
	return Info;
}

/** This is just some syntax sugar to avoid calling gode to have to IGameplayAbilitiesModule::Get() */
UAbilitySystemGlobals& UAbilitySystemGlobals::Get()
{
	static UAbilitySystemGlobals * GlobalPtr = IGameplayAbilitiesModule::Get().GetAbilitySystemGlobals();
	check(GlobalPtr == IGameplayAbilitiesModule::Get().GetAbilitySystemGlobals());
	check(GlobalPtr);

	return *GlobalPtr;
}
