// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneFloatSection.h"
#include "MovieSceneMaterialParameterSection.generated.h"

/**
 * Structure representing an animated scalar parameter and it's associated animation curve.
 */
USTRUCT()
struct FScalarParameterNameAndCurve
{
	GENERATED_USTRUCT_BODY()

	FScalarParameterNameAndCurve() { }

	/** Creates a new FScalarParameterNameAndCurve for a specific scalar parameter. */
	FScalarParameterNameAndCurve( FName InParameterName );

	/** The name of the scalar parameter which is being animated. */
	UPROPERTY()
	FName ParameterName;

	/** The curve which contains the animation data for the scalar parameter. */
	UPROPERTY()
	FRichCurve ParameterCurve;
};

/**
 * A movie scene section representing animation data for material parameters. 
 */
UCLASS( MinimalAPI )
class UMovieSceneMaterialParameterSection : public UMovieSceneSection
{
	GENERATED_UCLASS_BODY()

public:
	/** Adds a a key for a specific scalar parameter at the specified time with the specified value. */
	void AddScalarParameterKey(FName InParameterName, float InTime, float InValue);

	/** Gets the animated scalar parameters and their associated curves. */
	MOVIESCENETRACKS_API TArray<FScalarParameterNameAndCurve>* GetScalarParameterNamesAndCurves();

private:
	/** Gets a curve for a specific scalar parameter name. */
	FRichCurve* GetScalarParameterCurve( FName InParameterName );

private:
	/**
	 * The scalar parameter names and their associated curves.
	 * NOTE: This isn't stored in a map since we want order added preserved and random access doesn't need to be fast.
	 */
	UPROPERTY()
	TArray<FScalarParameterNameAndCurve> ScalarParameterNamesAndCurves;
};