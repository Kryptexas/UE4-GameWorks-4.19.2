// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UEnvQueryOption::UEnvQueryOption(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
}

FString UEnvQueryOption::GetDescriptionTitle() const
{
	return Generator ? Generator->GetDescriptionTitle() : FString();
}

FString UEnvQueryOption::GetDescriptionDetails() const
{
	return Generator ? Generator->GetDescriptionDetails() : FString();
}
