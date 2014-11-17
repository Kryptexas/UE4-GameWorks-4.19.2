// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneSection.h"

#include "MovieSceneMarginSection.generated.h"

/**
 * A section in a Margin track
 */
UCLASS(MinimalAPI)
class UMovieSceneMarginSection : public UMovieSceneSection
{
	GENERATED_UCLASS_BODY()
public:
	/** UMovieSceneSection interface */
	virtual void MoveSection( float DeltaPosition ) override;
	virtual void DilateSection( float DilationFactor, float Origin ) override;

	/**
	 * Updates this section
	 *
	 * @param Position	The position in time within the movie scene
	 */
	virtual FMargin Eval( float Position ) const;

	/** 
	 * Adds a key to the section
	 *
	 * @param Time	The location in time where the key should be added
	 * @param Value	The value of the key
	 */
	void AddKey( float Time, const FMargin& Value );
	
	/** 
	 * Determines if a new key would be new data, or just a duplicate of existing data
	 *
	 * @param Time	The location in time where the key would be added
	 * @param Value	The value of the new key
	 * @return True if the new key would be new data, false if duplicate
	 */
	bool NewKeyIsNewData(float Time, const FMargin& Value) const;

	/**
	 * Gets the top curve
	 *
	 * @return The rich curve for this margin channel
	 */
	FRichCurve& GetTopCurve() { return TopCurve; }
	const FRichCurve& GetTopCurve() const { return TopCurve; }

	/**
	 * Gets the left curve
	 *
	 * @return The rich curve for this margin channel
	 */
	FRichCurve& GetLeftCurve() { return LeftCurve; }
	const FRichCurve& GetLeftCurve() const { return LeftCurve; }
	/**
	 * Gets the right curve
	 *
	 * @return The rich curve for this margin channel
	 */
	FRichCurve& GetRightCurve() { return RightCurve; }
	const FRichCurve& GetRightCurve() const { return RightCurve; }
	
	/**
	 * Gets the bottom curve
	 *
	 * @return The rich curve for this margin channel
	 */
	FRichCurve& GetBottomCurve() { return BottomCurve; }
	const FRichCurve& GetBottomCurve() const { return BottomCurve; }

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
	FRichCurve TopCurve;

	/** Green curve data */
	UPROPERTY()
	FRichCurve LeftCurve;

	/** Blue curve data */
	UPROPERTY()
	FRichCurve RightCurve;

	/** Alpha curve data */
	UPROPERTY()
	FRichCurve BottomCurve;
};
