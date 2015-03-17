// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "ActorFactoryNote.generated.h"

UCLASS( MinimalAPI, config = Editor, collapsecategories, hidecategories = Object )
class UActorFactoryNote : public UActorFactory
{
	GENERATED_BODY()
public:
	UNREALED_API UActorFactoryNote(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

};
