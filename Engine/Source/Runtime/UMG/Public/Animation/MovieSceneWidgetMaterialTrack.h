// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneMaterialTrack.h"
#include "MovieSceneWidgetMaterialTrack.generated.h"


/**
 * A material track which is specialized for materials which are owned by widget brushes.
 */
UCLASS(MinimalAPI)
class UMovieSceneWidgetMaterialTrack
	: public UMovieSceneMaterialTrack
{
	GENERATED_UCLASS_BODY()

public:

	// UMovieSceneTrack interface

	virtual TSharedPtr<IMovieSceneTrackInstance> CreateInstance() override;
	virtual FName GetTrackName() const override { return BrushPropertyName; }

#if WITH_EDITORONLY_DATA
	virtual FText GetDefaultDisplayName() const override;
#endif

public:

	/** Gets name of the brush property which has the material to animate. */
	FName GetBrushPropertyName() const { return BrushPropertyName; }

	/** Sets the name of the brush property which has the material to animate. */
	void SetBrushPropertyName(FName InBrushPropertyName) 
	{
		BrushPropertyName = InBrushPropertyName;
	}

private:

	/** The name of the brush property which will be animated by this track. */
	UPROPERTY()
	FName BrushPropertyName;
};
