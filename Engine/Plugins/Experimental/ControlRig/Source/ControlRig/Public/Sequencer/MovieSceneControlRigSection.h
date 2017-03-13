// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Curves/KeyHandle.h"
#include "Sections/MovieSceneSubSection.h"
#include "ControlRig.h"
#include "MovieSceneSequencePlayer.h"
#include "MovieSceneControlRigSection.generated.h"

class UControlRigSequence;

/**
 * Movie scene section that controls animation controller animation
 */
UCLASS()
class CONTROLRIG_API UMovieSceneControlRigSection : public UMovieSceneSubSection
{
	GENERATED_BODY()

public:
	/** The weight curve for this animation controller section */
	UPROPERTY(EditAnywhere, Category = "Animation")
	FRichCurve Weight;

public:

	UMovieSceneControlRigSection();

	//~ MovieSceneSection interface

	virtual void MoveSection(float DeltaTime, TSet<FKeyHandle>& KeyHandles) override;
	virtual void DilateSection(float DilationFactor, float Origin, TSet<FKeyHandle>& KeyHandles) override;
	virtual void GetKeyHandles(TSet<FKeyHandle>& OutKeyHandles, TRange<float> TimeRange) const override;
};
