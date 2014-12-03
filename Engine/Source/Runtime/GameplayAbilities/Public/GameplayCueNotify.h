// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayTags.h"
#include "GameplayEffect.h"
#include "GameplayCueNotify.generated.h"


/**
 *	A self contained handler of a GameplayCue. These are similiar to AnimNotifies in implementation.
 *	
 *	These can be instanced or not, depending on ::NeedsInstanceForEvent.
 *	 
 */

UCLASS(abstract, BlueprintType, hidecategories=Object, collapsecategories)
class GAMEPLAYABILITIES_API UGameplayCueNotify : public UDataAsset
{
	GENERATED_UCLASS_BODY()

	/** Does this GameplayCueNotify handle this type of GameplayCueEvent? */
	virtual bool HandlesEvent(EGameplayCueEvent::Type EventType) const;

	/** Does this GameplayCueNotify need a per source actor instance for this event? */
	virtual bool NeedsInstanceForEvent(EGameplayCueEvent::Type EventType) const;

	virtual void OnOwnerDestroyed();

	virtual void PostInitProperties();

	virtual void Serialize(FArchive& Ar);
	
	virtual void HandleGameplayCue(AActor* MyTarget, EGameplayCueEvent::Type EventType, FGameplayCueParameters Parameters);

	UPROPERTY(EditDefaultsOnly, Category=GameplayCue, meta=(Categories="GameplayCue"))
	FGameplayTag	GameplayCueTag;

	/** Mirrors GameplayCueTag in order to be asset registry searchable */
	UPROPERTY(AssetRegistrySearchable)
	FName GameplayCueName;

	/** Does this Cue override other cues, or is it called in addition to them? E.g., If this is Damage.Physical.Slash, we wont call Damage.Physical afer we run this cue. */
	UPROPERTY(EditDefaultsOnly, Category=GameplayCue)
	bool IsOverride;

private:

	void	DeriveGameplayCueTagFromAssetName();	
};
