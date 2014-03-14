// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AppleMoviePlayerPrivatePCH.h"

#include "MoviePlayer.h"

TSharedPtr<FAVPlayerMovieStreamer> AppleMovieStreamer;

class FAppleMoviePlayerModule : public IModuleInterface
{
	/** IModuleInterface implementation */
	virtual void StartupModule() OVERRIDE
	{
		FAVPlayerMovieStreamer *Streamer = new FAVPlayerMovieStreamer;
		AppleMovieStreamer = MakeShareable(Streamer);
		GetMoviePlayer()->RegisterMovieStreamer(AppleMovieStreamer);
	}

	virtual void ShutdownModule() OVERRIDE
	{
		AppleMovieStreamer.Reset();
	}
};

IMPLEMENT_MODULE( FAppleMoviePlayerModule, AppleMoviePlayer )
