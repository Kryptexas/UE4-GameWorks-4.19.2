// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ExampleDeviceProfileSelectorModule.cpp: Implements the FExampleDeviceProfileSelectorModule class.
=============================================================================*/

#include "ExampleDeviceProfileSelectorPrivatePCH.h"

IMPLEMENT_MODULE(FExampleDeviceProfileSelectorModule, ExampleDeviceProfileSelector);


void FExampleDeviceProfileSelectorModule::StartupModule()
{
}


void FExampleDeviceProfileSelectorModule::ShutdownModule()
{
}


const FString FExampleDeviceProfileSelectorModule::GetRuntimeDeviceProfileName()
{
	FString ProfileName = FPlatformProperties::PlatformName();
	UE_LOG(LogInit, Log, TEXT("Selected Device Profile: [%s]"), *ProfileName);
	return ProfileName;
}