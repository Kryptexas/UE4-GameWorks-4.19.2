// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "EnvironmentQuery/EnvQueryGenerator.h"

UEnvQueryGenerator::UEnvQueryGenerator(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
}

FText UEnvQueryGenerator::GetDescriptionTitle() const
{
	return UEnvQueryTypes::GetShortTypeName(this);
}

FText UEnvQueryGenerator::GetDescriptionDetails() const
{
	return FText::GetEmpty();
}

#if WITH_EDITOR && USE_EQS_DEBUGGER
void UEnvQueryGenerator::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) 
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
#if USE_EQS_DEBUGGER
	UEnvQueryManager::NotifyAssetUpdate(NULL);
#endif
}
#endif //WITH_EDITOR && USE_EQS_DEBUGGER
