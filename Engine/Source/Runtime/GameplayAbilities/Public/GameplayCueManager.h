// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayTags.h"
#include "GameplayEffect.h"
#include "GameplayCueNotify.h"
#include "GameplayCueManager.generated.h"

/**
 *	
 *	Major TODO:
 *	
 *	-Remove LoadObjectLibraryFromPaths from base implementation. Just have project pass in ObjectLibrary, let it figure out how to make it.
 *		-On Bob's recommendation: in hopes of removing object libraries coupling with directory structure.
 *	
 *	-Async loading of all required GameplayCueNotifies
 *		-Currently async loaded on demand (first GameplayCueNotify is dropped)
 *		-Need way to enumerate all required GameplayCue tags (Interface? GameplayCueTags can come from GameplayEffects and GameplayAbilities, possibly more?)
 *		-Implemented UGameplayCueManager::BeginLoadingGameplayCueNotify
 *		
 *	-Better figure out how to handle instancing of GameplayCueNotifies
 *		-How to handle status type GameplayCueNotifies? (Code/blueprint will spawn stuff, needs to keep track and destroy them when GameplayCue is removed)
 *		-Instanced InstantiatedObjects is growing unbounded!
 *		 
 *	
 *	-Editor Workflow:
 *		-Make way to create new GameplayCueNotifies from UGameplayCueManager (details customization)
 *		-Jump to GameplayCueManager entry for GameplayCueTag (details customization)
 *		-Implement HandleAssetAdded and HandleAssetDeleted
 *			-Must make sure we update GameplayCueData/GameplayCueDataMap at appropriate times
 *				-On startup/begin PIE or try to do it as things change?
 *		
 *	-Overriding/forwarding: are we doing this right?
 *		-When can things override, when can they call parent implementations, etc
 *		-Do GameplayCueNotifies ever override GameplayCue Events or vice versa?
 *		
 *	-Take a pass on destruction
 *		-Make sure we are cleaning up GameplayCues when actors are destroyed
 *			(Register with Actor delegate or force game code to call EndGameplayCuesFor?)
 *	
 */


USTRUCT()
struct FGameplayCueNotifyData
{
	GENERATED_USTRUCT_BODY()

	FGameplayCueNotifyData()
	: LoadedGameplayCueNotify(nullptr)
	, ParentDataIdx( INDEX_NONE )
	{
	}

	UPROPERTY(VisibleDefaultsOnly, Category=GameplayCue, meta=(AllowedClasses="GameplayCueNotify"))
	FStringAssetReference	GameplayCueNotifyObj;

	UPROPERTY(transient)
	UGameplayCueNotify*		LoadedGameplayCueNotify;

	int32 ParentDataIdx;
};

/**
 *	A self contained handler of a GameplayCue. These are similiar to AnimNotifies in implementation.
 */

UCLASS()
class GAMEPLAYABILITIES_API UGameplayCueManager : public UDataAsset
{
	GENERATED_UCLASS_BODY()

	// -------------------------------------------------------------
	// Handling GameplayCues at runtime:
	// -------------------------------------------------------------

	virtual void HandleGameplayCues(AActor* TargetActor, const FGameplayTagContainer& GameplayCueTags, EGameplayCueEvent::Type EventType, FGameplayCueParameters Parameters);

	virtual void HandleGameplayCue(AActor* TargetActor, FGameplayTag GameplayCueTag, EGameplayCueEvent::Type EventType, FGameplayCueParameters Parameters);

	/** Force any instanced GameplayCueNotifies to stop */
	virtual void EndGameplayCuesFor(AActor* TargetActor);

	// -------------------------------------------------------------
	//  Loading GameplayCueNotifies from ObjectLibraries
	// -------------------------------------------------------------

	/** Loading soft refs to all GameplayCueNotifies */
	void LoadObjectLibraryFromPaths(const TArray<FString>& Paths, bool bFullyLoad);

	UPROPERTY(EditDefaultsOnly, Category = GameplayCue)
	TArray<FGameplayCueNotifyData>	GameplayCueData;

	/** Maps GameplayCue Tag to index into above GameplayCues array. */
	TMap<FGameplayTag, int32>	GameplayCueDataMap;
	
	UPROPERTY(transient)
	UObjectLibrary* GameplayCueNotifyObjectLibrary;

	// -------------------------------------------------------------
	// Preload GameplayCue tags that we think we will need:
	// -------------------------------------------------------------

	void	BeginLoadingGameplayCueNotify(FGameplayTag GameplayCueTag);

	int32	FinishLoadingGameplayCueNotifies();

	FStreamableManager	StreamableManager;

	UPROPERTY(transient)
	TArray<UGameplayCueNotify*>	LoadedObjects;

	UPROPERTY(transient)
	TArray<UGameplayCueNotify*>	InstantiatedObjects;

	// Fixme: we can combine the AActor* and the int32 into a single struct with a decent hash and avoid double map lookups
	TMap< TWeakObjectPtr<AActor>, TMap<int32, TWeakObjectPtr<UGameplayCueNotify> > >	NotifyMap;

	static FGameplayTag	BaseGameplayCueTag();

	void PrintGameplayCueNotifyMap();

#if WITH_EDITOR
	/** Handles updating an object library when a new asset is created */
	void HandleAssetAdded(UObject *Object);

	/** Handles cleaning up an object library if it matches the passed in object */
	void HandleAssetDeleted(UObject *Object);

	bool RegisteredEditorCallbacks;
#endif

private:

	virtual void HandleGameplayCueNotify_Internal(AActor* TargetActor, int32 DataIdx, EGameplayCueEvent::Type EventType, FGameplayCueParameters Parameters);
};