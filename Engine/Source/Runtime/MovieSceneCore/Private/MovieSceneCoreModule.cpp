// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneCorePrivatePCH.h"
#include "ModuleManager.h"

#include "MovieScene.h"
#include "MovieSceneBindings.h"
#include "RuntimeMovieScenePlayer.h"
#include "MovieSceneCore.generated.inl"


/**
 * MovieSceneCore module implementation (private)
 */
class FMovieSceneCoreModule : public IMovieSceneCore
{

public:

	/** IModuleInterface */
	virtual void StartupModule() OVERRIDE;
	virtual void ShutdownModule() OVERRIDE;

};


IMPLEMENT_MODULE( FMovieSceneCoreModule, MovieSceneCore );



void FMovieSceneCoreModule::StartupModule()
{
}


void FMovieSceneCoreModule::ShutdownModule()
{
}