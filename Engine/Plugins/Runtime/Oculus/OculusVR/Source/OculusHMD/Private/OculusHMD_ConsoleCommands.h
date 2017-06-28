// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "OculusHMDPrivate.h"

#if OCULUS_HMD_SUPPORTED_PLATFORMS
#include "HAL/IConsoleManager.h"

namespace OculusHMD
{

//-------------------------------------------------------------------------------------------------
// FConsoleCommands
//-------------------------------------------------------------------------------------------------

class FConsoleCommands
{
public:
	FConsoleCommands(class FOculusHMD* InHMDPtr);

private:
	FAutoConsoleCommand UpdateOnRenderThreadCommand;
	FAutoConsoleCommand PixelDensityCommand;
	FAutoConsoleCommand PixelDensityMinCommand;
	FAutoConsoleCommand PixelDensityMaxCommand;
	FAutoConsoleCommand PixelDensityAdaptiveCommand;
	FAutoConsoleCommand HQBufferCommand;
	FAutoConsoleCommand HQDistortionCommand;

#if !UE_BUILD_SHIPPING
	// Debug console commands
	FAutoConsoleCommand UpdateOnGameThreadCommand;
	FAutoConsoleCommand PositionOffsetCommand;
	FAutoConsoleCommand EnforceHeadTrackingCommand;
	FAutoConsoleCommand StatsCommand;
	FAutoConsoleCommand GridCommand;
	FAutoConsoleCommand CubemapCommand;
	FAutoConsoleCommand ShowSettingsCommand;
	FAutoConsoleCommand ResetSettingsCommand;
	FAutoConsoleCommand IPDCommand;
	FAutoConsoleCommand FCPCommand;
	FAutoConsoleCommand NCPCommand;
#endif // !UE_BUILD_SHIPPING
};


} // namespace OculusHMD

#endif //OCULUS_HMD_SUPPORTED_PLATFORMS
