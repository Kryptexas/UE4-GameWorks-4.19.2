// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.generated.h"

/** Simple struct for a gameplay tag container */
USTRUCT(BlueprintType)
struct GAMEPLAYTAGS_API FGameplayTagContainer
{
	GENERATED_USTRUCT_BODY()

public:

	/** Constructors */
	FGameplayTagContainer();
	FGameplayTagContainer(FGameplayTagContainer const& Other);

	/** Assignment/Equality operators */
	FGameplayTagContainer& operator=(FGameplayTagContainer const& Other);
	bool operator==(FGameplayTagContainer const& Other) const;
	bool operator!=(FGameplayTagContainer const& Other) const;

	/**
	 * Get the tags in the container
	 * 
	 * @param OutTags	[OUT] Tags in the container
	 */
	virtual void GetTags(OUT TSet<FName>& OutTags) const;

	/**
	 * Determine if the container has the specified tag
	 * 
	 * @param TagToCheck	Tag to check if it is present in the container
	 * 
	 * @return True if the tag is in the container, false if it is not
	 */
	virtual bool HasTag(FName TagToCheck) const;

	/**
	 * Checks if this container has ALL Tags.
	 * 
	 * @param Other Container we are checking against
	 * 
	 * @return True if this container has ALL the tags of the passed in container
	 */
	virtual bool HasAllTags(FGameplayTagContainer const& Other) const;

	/**
	 * Checks if this container has ANY Tags.
	 * 
	 * @param Other Container we are checking against
	 * 
	 * @return True if this container has ANY the tags of the passed in container
	 */
	virtual bool HasAnyTag(FGameplayTagContainer const& Other) const;

	/**
	 * Add the specified tag to the container
	 * 
	 * @param TagToAdd	Tag to add to the container
	 */
	virtual void AddTag(FName TagToAdd);

	/** 
	 * Adds all the tags from one container to this container 
	 *
	 * @param Other TagContainer that has the tags you want to add to this container 
	 */
	virtual void AppendTags(FGameplayTagContainer const& Other);

	/**
	 * Tag to remove from the container
	 * 
	 * @param TagToRemove	Tag to remove from the container
	 */
	virtual void RemoveTag(FName TagToRemove);

	/** Remove all tags from the container */
	virtual void RemoveAllTags();

	/**
	 * Serialize the tag container
	 *
	 * @param Ar	Archive to serialize to
	 *
	 * @return True if the container was serialized
	 */
	bool Serialize(FArchive& Ar);

	/** 
	 * Renames any tags that may have changed by the ini file
	 */
	void RedirectTags();

	/**
	 * Returns the Tag Count
	 *
	 * @return The number of tags
	 */
	int32 Num() const;

	/** 
	 * Returns human readable Tag list
	 */
	FString ToString() const;

	/** Array of gameplay tags */
	UPROPERTY(BlueprintReadWrite, Category=GameplayTags)
	TArray<FName> Tags;
};

template<>
struct TStructOpsTypeTraits<FGameplayTagContainer> : public TStructOpsTypeTraitsBase
{
	enum
	{
		WithSerializer = true,
		WithIdenticalViaEquality = true,
		WithCopy = true
	};
};
