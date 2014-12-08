// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "LogVisualizerDebugActor.generated.h"

/**
*	Transient actor used to draw visual logger data on level
*/

DECLARE_MULTICAST_DELEGATE_OneParam(FOnSelectionChanged, class AActor*);

UCLASS(config = Engine, NotBlueprintable, Transient)
class LOGVISUALIZER_API ALogVisualizerDebugActor : public AActor
{
	GENERATED_UCLASS_BODY()

};
