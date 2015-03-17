// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Commandlets/Commandlet.h"
#include "PluginCommandlet.generated.h"

UCLASS()
class UPluginCommandlet : public UCommandlet
{
	GENERATED_BODY()
public:
	UPluginCommandlet(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Parsed commandline tokens */
	TArray<FString> CmdLineTokens;

	/** Parsed commandline switches */
	TArray<FString> CmdLineSwitches;


	virtual int32 Main(const FString& Params) override;
};
