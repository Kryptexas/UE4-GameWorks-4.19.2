// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UEnvQueryGenerator::UEnvQueryGenerator(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
}

FString UEnvQueryGenerator::GetDescriptionTitle() const
{
	return UEnvQueryTypes::GetShortTypeName(this);
}

FString UEnvQueryGenerator::GetDescriptionDetails() const
{
	return TEXT("");
}

#if WITH_EDITOR
void UEnvQueryGenerator::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) 
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	UEnvQueryManager::NotifyAssetUpdate(NULL);
}
#endif //WITH_EDITOR
