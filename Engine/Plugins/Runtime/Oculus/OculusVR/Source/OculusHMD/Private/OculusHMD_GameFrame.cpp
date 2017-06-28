// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "OculusHMD_GameFrame.h"

#if OCULUS_HMD_SUPPORTED_PLATFORMS
#include "GameFramework/WorldSettings.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

namespace OculusHMD
{

//-------------------------------------------------------------------------------------------------
// FGameFrame
//-------------------------------------------------------------------------------------------------

FGameFrame::FGameFrame() :
	FrameNumber(0),
	WorldToMetersScale(100.f),
	PositionScale(1.0f),
	ShowFlags(ESFIM_All0),
	PlayerOrientation(FQuat::Identity),
	PlayerLocation(FVector::ZeroVector)
{
	Flags.Raw = 0;
}

TSharedPtr<FGameFrame, ESPMode::ThreadSafe> FGameFrame::Clone() const
{
	TSharedPtr<FGameFrame, ESPMode::ThreadSafe> NewFrame = MakeShareable(new FGameFrame(*this));
	return NewFrame;
}


void FGameFrame::SaveGamePose()
{
	FMemory::Memcpy(GameHeadPose, HeadPose);
	FMemory::Memcpy(GameEyePose[0], EyePose[0]);
	FMemory::Memcpy(GameEyePose[1], EyePose[1]);
	FMemory::Memcpy(GameEyePose[2], EyePose[2]);
}


void FGameFrame::ResetToGamePose()
{
	FMemory::Memcpy(HeadPose, GameHeadPose);
	FMemory::Memcpy(EyePose[0], GameEyePose[0]);
	FMemory::Memcpy(EyePose[1], GameEyePose[1]);
	FMemory::Memcpy(EyePose[2], GameEyePose[2]);
}


} // namespace OculusHMD

#endif //OCULUS_HMD_SUPPORTED_PLATFORMS
