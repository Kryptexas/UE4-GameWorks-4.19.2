// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieScenePrivatePCH.h"
#include "ModuleManager.h"

#include "MovieScene.h"
#include "MovieSceneBindings.h"
#include "RuntimeMovieScenePlayer.h"

DEFINE_LOG_CATEGORY(LogSequencerRuntime);


/**
 * MovieScene module implementation (private)
 */
class FMovieSceneModule : public IMovieSceneModule
{

public:

	/** IModuleInterface */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

};


IMPLEMENT_MODULE( FMovieSceneModule, MovieScene );



void FMovieSceneModule::StartupModule()
{
}


void FMovieSceneModule::ShutdownModule()
{
}