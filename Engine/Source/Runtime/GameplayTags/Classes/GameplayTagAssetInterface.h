// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "GameplayTagAssetInterface.generated.h"

/** Interface for assets which contain gameplay tags */
UINTERFACE(Blueprintable, MinimalAPI, meta=(CannotImplementInterfaceInBlueprint))
class UGameplayTagAssetInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class GAMEPLAYTAGS_API IGameplayTagAssetInterface
{
	GENERATED_IINTERFACE_BODY()

	/**
	 * Get any owned gameplay tags on the asset
	 * 
	 * @param OutTags	[OUT] Set of tags on the asset
	 */
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const=0;

	// TODO: REMOVE THIS
	/**
	 * Check if the specified owned tag is on the asset
	 * 
	 * @param TagToCheck	Tag to check for on the asset
	 * 
	 * @return True if the tag is on the asset, false if it is not
	 */
	virtual bool HasOwnedGameplayTag(FGameplayTag TagToCheck) const=0;

	/**
	 * Check if the asset has a gameplay tag that matches against the specified tag (expands to include parents of asset tags)
	 * 
	 * @param TagToCheck	Tag to check for a match
	 * 
	 * @return True if the asset has a gameplay tag that matches, false if not
	 */
	UFUNCTION(BlueprintCallable, Category=GameplayTags)
	virtual bool HasMatchingGameplayTag(FGameplayTag TagToCheck) const;

	/**
	 * Check if the asset has gameplay tags that matches against all of the specified tags (expands to include parents of asset tags)
	 * 
	 * @param TagContainer	Tag container to check for a match
	 * 
	 * @return True if the asset has matches all of the gameplay tags
	 */
	UFUNCTION(BlueprintCallable, Category=GameplayTags)
	virtual bool HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const;

	/**
	 * Check if the asset has gameplay tags that matches against any of the specified tags (expands to include parents of asset tags)
	 * 
	 * @param TagContainer	Tag container to check for a match
	 * 
	 * @return True if the asset has matches any of the gameplay tags
	 */
	UFUNCTION(BlueprintCallable, Category=GameplayTags)
	virtual bool HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const;
};

