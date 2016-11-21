// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieScenePropertyTrack.h"
#include "Slate/WidgetTransform.h"
#include "KeyParams.h"
#include "MovieScene2DTransformTrack.generated.h"


/**
 * Handles manipulation of 2D transforms in a movie scene
 */
UCLASS( MinimalAPI )
class UMovieScene2DTransformTrack : public UMovieScenePropertyTrack
{
	GENERATED_BODY()

public:

	UMovieScene2DTransformTrack(const FObjectInitializer& ObjectInitializer);
	
	// UMovieSceneTrack interface

	virtual UMovieSceneSection* CreateNewSection() override;
	virtual FMovieSceneEvalTemplatePtr CreateTemplateForSection(const UMovieSceneSection& InSection) const override;
	
public:

	DEPRECATED(4.15, "Please evaluate using FMovieScene2DTransformTemplate.")
	bool Eval( float Position, float LastPostion, FWidgetTransform& InOutTransform ) const;
};
