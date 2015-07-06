// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SequencerPossessedObject.generated.h"


/**
 * Structure for movie scene objects in Sequencer.
 */
USTRUCT()
struct FSequencerPossessedObject
{
	GENERATED_USTRUCT_BODY(FSequencerPossessedObject)

public:

	/** Creates and initializes a new instance. */
	FSequencerPossessedObject()
		: ObjectOrOwner(nullptr)
		, CachedComponent(nullptr)
	{ }

	/**
	 * Creates and initializes a new instance from an object.
	 *
	 * @param InObject The object to be bound.
	 */
	FSequencerPossessedObject(UObject* InObject)
		: ObjectOrOwner(InObject)
		, CachedComponent(nullptr)
	{ }

	/**
	 * Creates and initializes a new instance from an object component.
	 *
	 * @param InOwner The object that owns the component.
	 * @param InComponentName The component to be bound.
	 */
	FSequencerPossessedObject(UObject* InOwner, FString InComponentName)
		: ObjectOrOwner(InOwner)
		, ComponentName(InComponentName)
		, CachedComponent(nullptr)
	{ }

	/**
	 * Compares two bindings for equality.
	 *
	 * @param X The first binding to compare.
	 * @param Y The second binding to compare.
	 * @return true if the bindings refer to the same object, false otherwise.
	 */
	friend bool operator==(const FSequencerPossessedObject& X, const FSequencerPossessedObject& Y)
	{
		return (X.ObjectOrOwner == Y.ObjectOrOwner) && (X.ComponentName == Y.ComponentName);
	}

	/**
	 * Gets a pointer to the possessed object.
	 *
	 * @return The object (usually an Actor or an ActorComponent).
	 */
	SEQUENCER_API UObject* GetObject() const;

private:

	/** The object or the owner of the object being possessed. */
	UPROPERTY()
	TLazyObjectPtr<UObject> ObjectOrOwner;

	/** Optional name of an ActorComponent. */
	UPROPERTY()
	FString ComponentName;

	/** Cached pointer to the Actor component (only if ComponentName is set). */
	UPROPERTY(transient)
	mutable UObject* CachedComponent;
};
