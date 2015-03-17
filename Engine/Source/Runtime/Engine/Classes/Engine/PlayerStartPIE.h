// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// Player start for PIE - can be spawned during play.
//=============================================================================

#pragma once
#include "GameFramework/PlayerStart.h"
#include "PlayerStartPIE.generated.h"

UCLASS(notplaceable,MinimalAPI)
class APlayerStartPIE : public APlayerStart
{
	GENERATED_BODY()
public:
	ENGINE_API APlayerStartPIE(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

};



