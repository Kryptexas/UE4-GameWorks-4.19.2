// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "ActorFactoryTargetPoint.generated.h"

UCLASS( MinimalAPI, config = Editor, collapsecategories, hidecategories = Object )
class UActorFactoryTargetPoint : public UActorFactory
{
	GENERATED_BODY()
public:
	UNREALED_API UActorFactoryTargetPoint(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

};
