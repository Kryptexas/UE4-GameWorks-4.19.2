// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "GameplayCueNotify.h"

#pragma optimize( "", off )

UGameplayCueNotify::UGameplayCueNotify(const class FObjectInitializer& PCIP)
: Super(PCIP)
{
	IsOverride = true;
}

void UGameplayCueNotify::DeriveGameplayCueTagFromAssetName()
{
	// In the editor, attempt to infer GameplayCueTag from our asset name (if there is no valid GameplayCueTag already).
#if WITH_EDITOR
	if (GIsEditor)
	{
		if (GameplayCueTag.IsValid() == false)
		{
			FString MyName = GetName();

			MyName.RemoveFromStart(TEXT("Default__"));
			MyName.RemoveFromEnd(TEXT("_c"));

			MyName.ReplaceInline(TEXT("_"), TEXT("."));

			if (MyName.Contains(TEXT("GameplayCue")) == false)
			{
				MyName = FString(TEXT("GameplayCue.")) + MyName;
			}

			IGameplayTagsModule& GameplayTagsModule = IGameplayTagsModule::Get();
			GameplayCueTag = GameplayTagsModule.GetGameplayTagsManager().RequestGameplayTag(FName(*MyName), false);
		}

		GameplayCueName = GameplayCueTag.GetTagName();
	}
#endif
}

void UGameplayCueNotify::HandleGameplayCue(AActor *Self, EGameplayCueEvent::Type EventType, FGameplayCueParameters Parameters)
{

}

void UGameplayCueNotify::Serialize(FArchive& Ar)
{
	if (Ar.IsSaving())
	{
		DeriveGameplayCueTagFromAssetName();
	}
	
	Super::Serialize(Ar);

	if (Ar.IsLoading())
	{
		DeriveGameplayCueTagFromAssetName();
	}
}

void UGameplayCueNotify::PostInitProperties()
{
	Super::PostInitProperties();
	DeriveGameplayCueTagFromAssetName();
}

bool UGameplayCueNotify::HandlesEvent(EGameplayCueEvent::Type EventType) const
{
	return false;
}

/** Does this GameplayCueNotify need a per source actor instance for this event? */
bool UGameplayCueNotify::NeedsInstanceForEvent(EGameplayCueEvent::Type EventType) const
{
	return (EventType != EGameplayCueEvent::Executed);
}

void UGameplayCueNotify::OnOwnerDestroyed()
{
	// May need to do extra cleanup in child classes
	MarkPendingKill();
}