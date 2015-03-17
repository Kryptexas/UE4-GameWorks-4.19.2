// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "TickableAttributeSetInterface.generated.h"

/** Interface for actors which can be "spotted" by a player */
UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UTickableAttributeSetInterface : public UInterface
{
	GENERATED_BODY()
public:
	GAMEPLAYABILITIES_API UTickableAttributeSetInterface(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};

class ITickableAttributeSetInterface
{
	GENERATED_BODY()
public:

public:
	/**
	 * Ticks the attribute set by DeltaTime seconds
	 * 
	 * @param DeltaTime Size of the time step in seconds.
	 */
	virtual void Tick(float DeltaTime) = 0;

	/**
	* Does this attribute set need to tick?
	*
	* @return true if this attribute set should currently be ticking, false otherwise.
	*/
	virtual bool ShouldTick() const = 0;
};

