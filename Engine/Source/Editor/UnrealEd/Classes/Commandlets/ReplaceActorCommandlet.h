// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "ReplaceActorCommandlet.generated.h"

/** Commandlet for replacing one kind of actor with another kind of actor, copying changed properties from the most-derived common superclass */
UCLASS()
class UReplaceActorCommandlet : public UCommandlet
{
	GENERATED_UCLASS_BODY()
	// Begin UCommandlet Interface
	virtual int32 Main(const FString& Params) OVERRIDE;
	// End UCommandlet Interface
};


