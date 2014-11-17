// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "EnvironmentQuery/EnvQueryGenerator.h"
#include "EnvironmentQuery/EnvQueryOption.h"

UEnvQueryOption::UEnvQueryOption(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
}

FText UEnvQueryOption::GetDescriptionTitle() const
{
	return Generator ? Generator->GetDescriptionTitle() : FText::GetEmpty();
}

FText UEnvQueryOption::GetDescriptionDetails() const
{
	return Generator ? Generator->GetDescriptionDetails() : FText::GetEmpty();
}
