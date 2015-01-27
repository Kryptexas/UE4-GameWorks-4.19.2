// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/EngineTypes.h"
#include "LocalizationTarget.h"
#include "ProjectLocalizationSettings.generated.h"

UCLASS(Config=Game, defaultconfig)
class UProjectLocalizationSettings : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(config, EditAnywhere, Category = "LocalizationServiceProvider")
	FName LocalizationServiceProvider;

	UPROPERTY(EditAnywhere, Category = "Targets")
	TArray<ULocalizationTarget*> TargetObjects;

private:
	UPROPERTY(config)
	TArray<FLocalizationTargetSettings> Targets;

public:
#if WITH_EDITOR
	virtual void PostInitProperties() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};