// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "ActorFactorySkyLight.generated.h"

UCLASS(MinimalAPI, config=Editor, collapsecategories, hidecategories=Object)
class UActorFactorySkyLight : public UActorFactory
{
	GENERATED_BODY()
public:
	UNREALED_API UActorFactorySkyLight(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

};


