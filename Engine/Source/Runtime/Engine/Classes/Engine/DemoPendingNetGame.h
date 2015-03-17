// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "DemoPendingNetGame.generated.h"


UCLASS(transient, config=Engine)
class UDemoPendingNetGame
	: public UPendingNetGame
{
	GENERATED_BODY()
public:
	UDemoPendingNetGame(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
