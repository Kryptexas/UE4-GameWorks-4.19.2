// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayTags.h"
#include "GameplayCueView.generated.h"

/** 
	This is meant to give a base implementation for handling GameplayCues. It will handle very simple things like
	creating and destroying ParticleSystemComponents, playing audio, etc.

	It is inevitable that specific actors and games will have to handle GameplayCues in their own way.
*/

UENUM(BlueprintType)
namespace EGameplayCueEvent
{
	enum Type
	{
		Applied,
		Added,
		Executed,
		Removed
	};
}

USTRUCT()
struct FGameplayCueViewEffects
{
	// The instantied effects
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TWeakObjectPtr<UParticleSystemComponent> ParticleSystemComponent;

	UPROPERTY()
	TWeakObjectPtr<UAudioComponent>	AudioComponent;

	UPROPERTY()
	TWeakObjectPtr<AActor>	SpawnedActor;
};

USTRUCT()
struct FGameplayCueViewInfo
{
	GENERATED_USTRUCT_BODY()

	// -----------------------------
	//	Qualifiers
	// -----------------------------

	UPROPERTY(EditDefaultsOnly, Category = GameplayCue)
	FGameplayTagContainer Tags;

	UPROPERTY(EditDefaultsOnly, Category = GameplayModifier)
	TEnumAsByte<EGameplayCueEvent::Type> CueType;

	// This needs to be fleshed out more:
	bool	InstigatorLocalOnly;
	bool	TargetLocalOnly;

	// -----------------------------
	//	Effects
	// -----------------------------

	/** Local/remote sounds to play for weapon attacks against specific surfaces */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameplayCue)
	class USoundBase* Sound;

	/** Effects to play for weapon attacks against specific surfaces */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameplayCue)
	class UParticleSystem* ParticleSystem;

	/** Effects to play for weapon attacks against specific surfaces */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameplayCue)
	class TSubclassOf<AActor> ActorClass;

	virtual TSharedPtr<FGameplayCueViewEffects> SpawnViewEffects(AActor *Owner, TArray<UObject*> *SpawnedObjects) const;
};

UCLASS(BlueprintType)
class SKILLSYSTEM_API UGameplayCueView : public UDataAsset
{
	// Defines how a GameplayCue is translated into viewable components
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditDefaultsOnly, Category = GameplayCue)
	TArray<FGameplayCueViewInfo>	Views;
};

USTRUCT()
struct SKILLSYSTEM_API FGameplayCueHandler
{
	GENERATED_USTRUCT_BODY()

	FGameplayCueHandler()
		: Owner(NULL)
	{
	}

	UPROPERTY()
	AActor * Owner;

	UPROPERTY(EditDefaultsOnly, Category = GameplayCue)
	TArray<UGameplayCueView*>	Definitions;
	
	TMap<FName, TArray< TSharedPtr<FGameplayCueViewEffects> > >	SpawnedViewEffects;

	UPROPERTY()
	TArray<UObject*> SpawnedObjects;

	virtual void GameplayCueActivated(const FGameplayTagContainer & GameplayCueTags, float NormalizedMagnitude);
	
	virtual void GameplayCueExecuted(const FGameplayTagContainer & GameplayCueTags, float NormalizedMagnitude);
	
	virtual void GameplayCueAdded(const FGameplayTagContainer & GameplayCueTags, float NormalizedMagnitude);
	
	virtual void GameplayCueRemoved(const FGameplayTagContainer & GameplayCueTags, float NormalizedMagnitude);

private:

	void ClearEffects(TArray< TSharedPtr<FGameplayCueViewEffects > > &Effects);


};