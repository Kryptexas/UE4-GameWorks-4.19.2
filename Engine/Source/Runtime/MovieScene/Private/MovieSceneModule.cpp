// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "IMovieSceneModule.h"
#include "MovieScene.h"


DEFINE_LOG_CATEGORY(LogMovieScene);


/**
 * MovieScene module implementation.
 */
class FMovieSceneModule
	: public IMovieSceneModule
{
public:

	// IModuleInterface interface

	virtual void StartupModule() override { }
	virtual void ShutdownModule() override { }
};


IMPLEMENT_MODULE(FMovieSceneModule, MovieScene);
