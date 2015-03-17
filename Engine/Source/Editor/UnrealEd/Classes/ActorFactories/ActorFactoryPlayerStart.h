// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "ActorFactoryPlayerStart.generated.h"

UCLASS(MinimalAPI, config=Editor, collapsecategories, hidecategories=Object)
class UActorFactoryPlayerStart : public UActorFactory
{
	GENERATED_BODY()
public:
	UNREALED_API UActorFactoryPlayerStart(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

};



