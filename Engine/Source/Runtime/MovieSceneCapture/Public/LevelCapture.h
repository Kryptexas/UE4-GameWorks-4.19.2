// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneCapture.h"
#include "LevelCapture.generated.h"

UCLASS()
class MOVIESCENECAPTURE_API ULevelCapture : public UMovieSceneCapture
{
public:
	GENERATED_BODY()

	/** The level we want to load and capture */
	UPROPERTY(EditAnywhere, Category="General", meta=(AllowedClasses="World"))
	FStringAssetReference Level;

	virtual FString GetPackageName() const override { return Level.ToString(); }
	virtual void Initialize(FViewport* InViewport) override;
};