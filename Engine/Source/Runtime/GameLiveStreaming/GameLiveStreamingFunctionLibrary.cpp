// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "GameLiveStreamingModule.h"
#include "GameLiveStreamingFunctionLibrary.h"
#include "Public/IGameLiveStreaming.h"

#define LOCTEXT_NAMESPACE "GameLiveStreaming"


UGameLiveStreamingFunctionLibrary::UGameLiveStreamingFunctionLibrary( const FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
{
}


bool UGameLiveStreamingFunctionLibrary::IsBroadcastingGame()
{
	return IGameLiveStreaming::Get().IsBroadcastingGame();
}


void UGameLiveStreamingFunctionLibrary::StartBroadcastingGame(
		int32 FrameRate,
		float ScreenScaling,
		bool bEnableWebCam,
		int32 DesiredWebCamWidth,
		int32 DesiredWebCamHeight,
		bool bCaptureAudioFromComputer,
		bool bCaptureAudioFromMicrophone,
		bool bDrawSimpleWebCamVideo )
{
	FGameBroadcastConfig Config;
	Config.FrameRate = FrameRate;
	Config.ScreenScaling = ScreenScaling;
	Config.bEnableWebCam = bEnableWebCam;
	Config.DesiredWebCamWidth = DesiredWebCamWidth;
	Config.DesiredWebCamHeight = DesiredWebCamHeight;
	Config.bCaptureAudioFromComputer = bCaptureAudioFromComputer;
	Config.bCaptureAudioFromMicrophone = bCaptureAudioFromMicrophone;
	Config.bDrawSimpleWebCamVideo = bDrawSimpleWebCamVideo;
	IGameLiveStreaming::Get().StartBroadcastingGame( Config );
}


void UGameLiveStreamingFunctionLibrary::StopBroadcastingGame()
{
	IGameLiveStreaming::Get().StopBroadcastingGame();
}



#undef LOCTEXT_NAMESPACE
