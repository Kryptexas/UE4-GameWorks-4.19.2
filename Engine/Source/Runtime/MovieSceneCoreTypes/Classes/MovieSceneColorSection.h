// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneColorSection.generated.h"

/**
 * A single floating point section
 */
UCLASS( MinimalAPI )
class UMovieSceneColorSection : public UMovieSceneSection
{
	GENERATED_UCLASS_BODY()
public:
	/** MovieSceneSection interface */
	virtual void MoveSection( float DeltaPosition ) OVERRIDE;
	virtual void DilateSection( float DilationFactor, float Origin ) OVERRIDE;

	/**
	 * Updates this section
	 *
	 * @param Position	The position in time within the movie scene
	 */
	virtual FLinearColor Eval( float Position ) const;

	/** 
	 * Adds a key to the section
	 *
	 * @param Time	The location in time where the key should be added
	 * @param Value	The value of the key
	 */
	void AddKey( float Time, FLinearColor Value );
	
	/** 
	 * Determines if a new key would be new data, or just a duplicate of existing data
	 *
	 * @param Time	The location in time where the key would be added
	 * @param Value	The value of the new key
	 * @return True if the new key would be new data, false if duplicate
	 */
	bool NewKeyIsNewData(float Time, FLinearColor Value) const;

	/**
	 * Gets the red color curve
	 *
	 * @return The rich curve for this color channel
	 */
	FRichCurve& GetRedCurve() { return RedCurve; }
	const FRichCurve& GetRedCurve() const { return RedCurve; }

	/**
	 * Gets the green color curve
	 *
	 * @return The rich curve for this color channel
	 */
	FRichCurve& GetGreenCurve() { return GreenCurve; }
	const FRichCurve& GetGreenCurve() const { return GreenCurve; }
	/**
	 * Gets the blue color curve
	 *
	 * @return The rich curve for this color channel
	 */
	FRichCurve& GetBlueCurve() { return BlueCurve; }
	const FRichCurve& GetBlueCurve() const { return BlueCurve; }
	
	/**
	 * Gets the alpha color curve
	 *
	 * @return The rich curve for this color channel
	 */
	FRichCurve& GetAlphaCurve() { return AlphaCurve; }
	const FRichCurve& GetAlphaCurve() const { return AlphaCurve; }

private:
	/**
	 * Adds a key to a rich curve, finding an existing key to modify or adding a new one
	 *
	 * @param InCurve	The curve to add keys to
	 * @param Time		The time where the key should be added
	 * @param Value		The value at the given time
	 */
	void AddKeyToCurve( FRichCurve& InCurve, float Time, float Value );
private:
	/** Red curve data */
	UPROPERTY()
	FRichCurve RedCurve;

	/** Green curve data */
	UPROPERTY()
	FRichCurve GreenCurve;

	/** Blue curve data */
	UPROPERTY()
	FRichCurve BlueCurve;

	/** Alpha curve data */
	UPROPERTY()
	FRichCurve AlphaCurve;
};
