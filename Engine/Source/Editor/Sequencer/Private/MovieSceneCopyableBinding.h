// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "MovieSceneSpawnable.h"
#include "MovieSceneBinding.h"
#include "MovieScenePossessable.h"
#include "MovieSceneCopyableBinding.generated.h"

class UMovieSceneTrack;

UCLASS()
class UMovieSceneCopyableBinding : public UObject
{
public:
	GENERATED_BODY()
	
	UPROPERTY()
	UObject* SpawnableObjectTemplate;

	UPROPERTY()
	TArray<UMovieSceneTrack*> Tracks;

	UPROPERTY()
	FMovieSceneBinding Binding;

	UPROPERTY()
	FMovieSceneSpawnable Spawnable;

	UPROPERTY()
	FMovieScenePossessable Possessable;
};
