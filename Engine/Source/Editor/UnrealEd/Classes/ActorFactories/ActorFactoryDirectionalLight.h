// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "ActorFactoryDirectionalLight.generated.h"

UCLASS(MinimalAPI, config=Editor, collapsecategories, hidecategories=Object)
class UActorFactoryDirectionalLight : public UActorFactory
{
	GENERATED_BODY()
public:
	UNREALED_API UActorFactoryDirectionalLight(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

};



