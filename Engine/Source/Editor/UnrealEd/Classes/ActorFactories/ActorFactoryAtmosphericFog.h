// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "ActorFactoryAtmosphericFog.generated.h"

UCLASS(MinimalAPI, config=Editor, collapsecategories, hidecategories=Object)
class UActorFactoryAtmosphericFog : public UActorFactory
{
	GENERATED_BODY()
public:
	UNREALED_API UActorFactoryAtmosphericFog(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};



