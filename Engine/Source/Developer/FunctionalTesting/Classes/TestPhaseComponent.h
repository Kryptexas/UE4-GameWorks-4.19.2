// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Components/SceneComponent.h"
#include "TestPhaseComponent.generated.h"

UCLASS(hidecategories=Object, editinlinenew)
class UTestPhaseComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UTestPhaseComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//virtual void Start();
};