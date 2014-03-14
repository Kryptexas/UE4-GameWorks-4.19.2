// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayTagAssetInterface.generated.h"

/** Interface for assets which contain gameplay tags */
UINTERFACE(MinimalAPI)
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
	virtual void GetOwnedGameplayTags(TSet<FName>& OutTags) const=0;

	/**
	 * Check if the specified owned tag is on the asset
	 * 
	 * @param TagToCheck	Tag to check for on the asset
	 * 
	 * @return True if the tag is on the asset, false if it is not
	 */
	virtual bool HasOwnedGameplayTag(FName TagToCheck) const=0;
};

