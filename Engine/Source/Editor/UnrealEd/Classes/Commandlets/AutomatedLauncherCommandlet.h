// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/** Commandlet for running automated commands through the launcher */

#pragma once
#include "AutomatedLauncherCommandlet.generated.h"

UCLASS()
class UAutomatedLauncherCommandlet : public UCommandlet
{
	GENERATED_UCLASS_BODY()

	// Begin UCommandlet Interface
	virtual int32 Main(const FString& Params) OVERRIDE;
	// End UCommandlet Interface
};