// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneTransformSection.generated.h"

/**
 * A transform section
 */
UCLASS( DependsOn=UMovieSceneSection, MinimalAPI )
class UMovieSceneTransformSection : public UMovieSceneSection
{
	GENERATED_UCLASS_BODY()
public:

	/** MovieSceneSection interface */
	virtual void MoveSection( float DeltaPosition ) OVERRIDE;
	virtual void DilateSection( float DilationFactor, float Origin ) OVERRIDE;

	/**
	 * Evaluates the translation component of the transform
	 *
	 * @param Time				The position in time within the movie scene
	 * @param OutTranslation	The evaluated translation.  Note: will remain unchanged if there were no keys to evaluate
	 * @return true if there were any keys to evaluate.  
	 */
	bool EvalTranslation( float Time, FVector& OutTranslation ) const;

	/**
	 * Evaluates the rotation component of the transform
	 *
	 * @param Time				The position in time within the movie scene
	 * @param OutRotation		The evaluated rotation.  Note: will remain unchanged if there were no keys to evaluate
	 * @return true if there were any keys to evaluate.  
	 */
	bool EvalRotation( float Time, FRotator& OutRotation ) const;

	/**
	 * Evaluates the scale component of the transform
	 *
	 * @param Time				The position in time within the movie scene
	 * @param OutScale			The evaluated scale.  Note: will remain unchanged if there were no keys to evaluate
	 * @return true if there were any keys to evaluate.  
	 */
	bool EvalScale( float Time, FVector& OutScale ) const;

	/** 
	 * Returns the translation curve for a specific axis
	 *
	 * @param Axis	The axis of the translation curve to get
	 * @return The curve on the axis
	 */
	virtual FRichCurve& GetTranslationCurve( EAxis::Type Axis );

	/** 
	 * Returns the rotation curve for a specific axis
	 *
	 * @param Axis	The axis of the rotation curve to get
	 * @return The curve on the axis
	 */
	virtual FRichCurve& GetRotationCurve( EAxis::Type Axis );

	/** 
	 * Returns the scale curve for a specific axis
	 *
	 * @param Axis	The axis of the scale curve to get
	 * @return The curve on the axis
	 */
	virtual FRichCurve& GetScaleCurve( EAxis::Type Axis );

	/**
	 * Adds keys each component of translation. Note: Only adds keys if a value has changed
	 *
	 * @param TransformKey	Information about the transform that should be keyed
	 */
	void AddTranslationKeys( const class FTransformKey& TransformKey );

	/**
	 * Adds keys each component of rotation. Note: Only adds keys if a value has changed
	 *
	 * @param TransformKey	Information about the transform that should be keyed
	 * @param bUnwindRotation	When true, the value will be treated like a rotation value in degrees, and will automatically be unwound to prevent flipping 360 degrees from the previous key 	 
	 */
	void AddRotationKeys( const class FTransformKey& TransformKey, const bool bUnwindRotation );

	/**
	 * Adds keys each component of scale. Note: Only adds keys if a value has changed
	 *
	 * @param TransformKey	Information about the transform that should be keyed
	 */
	void AddScaleKeys( const class FTransformKey& TransformKey );
	
	/** 
	 * Determines if a new key would be new data, or just a duplicate of existing data
	 *
	 * @param TransformKey The new key that would be added
	 * @return True if the new key would be new data, false if duplicate
	 */
	bool NewKeyIsNewData(const class FTransformKey& TransformKey) const;
private:
	/**
	 * Adds a key to a rich curve, finding an existing key to modify or adding a new one
	 *
	 * @param InCurve	The curve to add keys to
	 * @param Time		The time where the key should be added
	 * @param Value		The value at the given time
	 * @param	bUnwindRotation		When true, the value will be treated like a rotation value in degrees, and will automatically be unwound to prevent flipping 360 degrees from the previous key 
	 */
	void AddKeyToCurve( FRichCurve& InCurve, float Time, float Value, const bool bUnwindRotation );
private:
	/** Translation curves */
	UPROPERTY()
	FRichCurve Translation[3];
	
	/** Rotation curves */
	UPROPERTY()
	FRichCurve Rotation[3];

	/** Scale curves */
	UPROPERTY()
	FRichCurve Scale[3];
};