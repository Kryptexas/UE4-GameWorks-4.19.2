// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// SpectatorPawns are simple pawns that can fly around the world, used by
// PlayerControllers when in the spectator state.
//
//=============================================================================

#pragma once
#include "SpectatorPawn.generated.h"


UCLASS(HeaderGroup=Pawn, config=Game, Blueprintable, BlueprintType)
class ENGINE_API ASpectatorPawn : public ADefaultPawn
{
	GENERATED_UCLASS_BODY()

	// Begin Pawn overrides
	/** Overridden to avoid changing network role. If subclasses want networked behavior, call the Pawn::PossessedBy() instead. */
	virtual void PossessedBy(class AController* NewController) OVERRIDE;
	// End Pawn overrides
};
