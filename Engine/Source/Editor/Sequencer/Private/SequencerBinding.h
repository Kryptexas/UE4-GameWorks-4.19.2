// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SequencerBinding.generated.h"


/**
 * Structure for object bindings.
 */
USTRUCT()
struct FSequencerBinding
{
	GENERATED_USTRUCT_BODY(FSequencerBinding)

public:

	/** Creates and initializes a new instance. */
	FSequencerBinding()
		: Object(nullptr)
		, CachedComponent(nullptr)
	{ }

	/**
	 * Creates and initializes an object binding.
	 *
	 * @param InObject The object to be bound.
	 */
	FSequencerBinding(UObject* InObject)
		: Object(InObject)
		, CachedComponent(nullptr)
	{ }

	/**
	 * Creates and initializes a component binding.
	 *
	 * @param InObject The object that owns the component.
	 * @param InComponentName The component to be bound.
	 */
	FSequencerBinding(UObject* InObject, FString InComponentName)
		: Object(InObject)
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
	friend bool operator==(const FSequencerBinding& X, const FSequencerBinding& Y)
	{
		return (X.Object == Y.Object) && (X.ComponentName == Y.ComponentName);
	}

	/**
	 * Gets a pointer to the bound object.
	 *
	 * @return The bound object.
	 */
	SEQUENCER_API UObject* GetBoundObject() const;

private:

	/** The object being bound, usually an Actor. */
	UPROPERTY()
	UObject* Object; // @todo sequencer: gmp: this needs to become a persistent object pointer

	/** Optional name of the ActorComponent being bound. */
	UPROPERTY()
	FString ComponentName;

	/** Cached pointer to the Actor component, if bound. */
	UPROPERTY(transient)
	mutable UObject* CachedComponent;
};
