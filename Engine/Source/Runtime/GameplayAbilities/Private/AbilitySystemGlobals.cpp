// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GameplayTagsModule.h"
#include "GameplayCueInterface.h"

UAbilitySystemGlobals::UAbilitySystemGlobals(const class FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{
	RegisteredReimportCallback = false;
}

void UAbilitySystemGlobals::InitGlobalData()
{
	GetGlobalCurveTable();
	GetGlobalAttributeMetaDataTable();
	
	InitAtributeDefaults();
}

UCurveTable * UAbilitySystemGlobals::GetGlobalCurveTable()
{
	return InternalGetLoadTable<UCurveTable>(GlobalCurveTable, GlobalCurveTableName);
}

UDataTable * UAbilitySystemGlobals::GetGlobalAttributeMetaDataTable()
{
	return InternalGetLoadTable<UDataTable>(GlobalAttributeMetaDataTable, GlobalAttributeMetaDataTableName);
}

template <class T>
T* UAbilitySystemGlobals::InternalGetLoadTable(T*& Table, FString TableName)
{
	if (!Table && !TableName.IsEmpty())
	{
		Table = LoadObject<T>(NULL, *TableName, NULL, LOAD_None, NULL);
#if WITH_EDITOR
		// Hook into notifications for object re-imports so that the gameplay tag tree can be reconstructed if the table changes
		if (GIsEditor && Table && !RegisteredReimportCallback)
		{
			GEditor->OnObjectReimported().AddUObject(this, &UAbilitySystemGlobals::OnTableReimported);
			RegisteredReimportCallback = true;
		}
#endif
	}

	return Table;
}


#if WITH_EDITOR

void UAbilitySystemGlobals::OnTableReimported(UObject* InObject)
{
	if (GIsEditor && !IsRunningCommandlet() && InObject && InObject == GlobalCurveTable)
	{
	}
	
	if (GIsEditor && !IsRunningCommandlet() && InObject && InObject == GlobalAttributeDefaultsTable)
	{
	}

	if (GIsEditor && !IsRunningCommandlet() && InObject && InObject == GlobalAttributeMetaDataTable)
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

FGameplayAbilityActorInfo * UAbilitySystemGlobals::AllocAbilityActorInfo() const
{
	return new FGameplayAbilityActorInfo();
}

/** This is just some syntax sugar to avoid calling gode to have to IGameplayAbilitiesModule::Get() */
UAbilitySystemGlobals& UAbilitySystemGlobals::Get()
{
	static UAbilitySystemGlobals * GlobalPtr = IGameplayAbilitiesModule::Get().GetAbilitySystemGlobals();
	check(GlobalPtr == IGameplayAbilitiesModule::Get().GetAbilitySystemGlobals());
	check(GlobalPtr);

	return *GlobalPtr;
}


// --------------------------------------------------------------------

UFunction* UAbilitySystemGlobals::GetGameplayCueFunction(const FGameplayTag& ChildTag, UClass* Class, FName &MatchedTag)
{
	SCOPE_CYCLE_COUNTER(STAT_GetGameplayCueFunction);

	// A global cached map to lookup these functions might be a good idea. Keep in mind though that FindFunctionByName
	// is fast and already gives us a reliable map lookup.
	// 
	// We would get some speed by caching off the 'fully qualified name' to 'best match' lookup. E.g. we can directly map
	// 'GameplayCue.X.Y' to the function 'GameplayCue.X' without having to look for GameplayCue.X.Y first.
	// 
	// The native remapping (Gameplay.X.Y to Gameplay_X_Y) is also annoying and slow and could be fixed by this as well.
	// 
	// Keep in mind that any UFunction* cacheing is pretty unsafe. Classes can be loaded (and unloaded) during runtime
	// and will be regenerated all the time in the editor. Just doing a single pass at startup won't be enough,
	// we'll need a mechanism for registering classes when they are loaded on demand.
	
	IGameplayTagsModule& GameplayTagsModule = IGameplayTagsModule::Get();
	FGameplayTagContainer TagAndParentsContainer = GameplayTagsModule.GetGameplayTagsManager().RequestGameplayTagParents(ChildTag);

	for (auto InnerTagIt = TagAndParentsContainer.CreateConstIterator(); InnerTagIt; ++InnerTagIt)
	{
		FName CueName = InnerTagIt->GetTagName();
		if (UFunction* Func = Class->FindFunctionByName(CueName, EIncludeSuperFlag::IncludeSuper))
		{
			MatchedTag = CueName;
			return Func;
		}

		// Native functions cant be named with ".", so look for them with _. 
		FName NativeCueFuncName = *CueName.ToString().Replace(TEXT("."), TEXT("_"));
		if (UFunction* Func = Class->FindFunctionByName(NativeCueFuncName, EIncludeSuperFlag::IncludeSuper))
		{
			MatchedTag = CueName; // purposefully returning the . qualified name.
			return Func;
		}
	}

	return nullptr;
}

// --------------------------------------------------------------------

/** Initialize FAttributeSetInitter. This is virtual so projects can override what class they use */
void UAbilitySystemGlobals::AllocAttributeSetInitter()
{
	GlobalAttributeSetInitter = TSharedPtr<FAttributeSetInitter>(new FAttributeSetInitter());
}

FAttributeSetInitter* UAbilitySystemGlobals::GetAttributeSetInitter() const
{
	check(GlobalAttributeSetInitter.IsValid());
	return GlobalAttributeSetInitter.Get();
}

void UAbilitySystemGlobals::InitAtributeDefaults()
{
	if (InternalGetLoadTable<UCurveTable>(GlobalAttributeDefaultsTable, GlobalAttributeSetDefaultsTableName))
	{	
		AllocAttributeSetInitter();
		GlobalAttributeSetInitter->PreloadAttributeSetData(GlobalAttributeDefaultsTable);
	}

}