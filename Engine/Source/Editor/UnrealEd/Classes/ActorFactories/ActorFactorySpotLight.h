// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "ActorFactorySpotLight.generated.h"

UCLASS(MinimalAPI, config=Editor, collapsecategories, hidecategories=Object)
class UActorFactorySpotLight : public UActorFactory
{
	GENERATED_BODY()
public:
	UNREALED_API UActorFactorySpotLight(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

};



