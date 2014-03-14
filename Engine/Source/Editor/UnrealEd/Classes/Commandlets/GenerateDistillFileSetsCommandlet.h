// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 *	Find AnimNotify instances that do not have the AnimSeqeunce that 'owns' them as their outer.
 */

#pragma once
#include "GenerateDistillFileSetsCommandlet.generated.h"

UCLASS()
class UGenerateDistillFileSetsCommandlet : public UCommandlet
{
	GENERATED_UCLASS_BODY()

	// Begin UCommandlet Interface
	virtual int32 Main(const FString& Params) OVERRIDE;
	// End UCommandlet Interface
};
