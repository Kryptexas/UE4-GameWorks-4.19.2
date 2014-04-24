// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

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

#if WITH_EDITOR
void UEnvQueryGenerator::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) 
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	UEnvQueryManager::NotifyAssetUpdate(NULL);
}
#endif //WITH_EDITOR
