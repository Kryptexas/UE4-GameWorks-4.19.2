// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/**
 *	Find AnimNotify instances that do not have the AnimSeqeunce that 'owns' them as their outer.
 */

#pragma once
#include "Commandlets/Commandlet.h"
#include "GenerateDistillFileSetsCommandlet.generated.h"

UCLASS()
class UGenerateDistillFileSetsCommandlet : public UCommandlet
{
	GENERATED_BODY()
public:
	UGenerateDistillFileSetsCommandlet(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Begin UCommandlet Interface
	virtual int32 Main(const FString& Params) override;
	// End UCommandlet Interface
};
