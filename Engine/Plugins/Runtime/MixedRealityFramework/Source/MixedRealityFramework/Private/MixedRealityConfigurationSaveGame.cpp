// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "MixedRealityConfigurationSaveGame.h"

UMixedRealityCalibrationData::UMixedRealityCalibrationData(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{}

UMixedRealityConfigurationSaveGame::UMixedRealityConfigurationSaveGame(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SaveSlotName = TEXT("MixedRealityConfigurationSaveSlot");
	UserIndex = 0;
	ConfigurationSaveVersion = 0;
}
