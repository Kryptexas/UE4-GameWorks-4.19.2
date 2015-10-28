// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneCapture.h"
#include "LevelCapture.generated.h"

UCLASS()
class MOVIESCENECAPTURE_API ULevelCapture : public UMovieSceneCapture
{
public:
	GENERATED_BODY()
	virtual void Initialize(TWeakPtr<FSceneViewport> InViewport) override;
};