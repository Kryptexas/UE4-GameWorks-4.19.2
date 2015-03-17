// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "ActorFactoryPointLight.generated.h"

UCLASS(MinimalAPI, config=Editor, collapsecategories, hidecategories=Object)
class UActorFactoryPointLight : public UActorFactory
{
	GENERATED_BODY()
public:
	UNREALED_API UActorFactoryPointLight(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

};



