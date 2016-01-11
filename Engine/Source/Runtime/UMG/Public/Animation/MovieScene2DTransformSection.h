// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneSection.h"

#include "MovieScene2DTransformSection.generated.h"

struct FWidgetTransform;

/**
 * A transform section
 */
UCLASS(MinimalAPI)
class UMovieScene2DTransformSection : public UMovieSceneSection
{
	GENERATED_UCLASS_BODY()
public:

	/** MovieSceneSection interface */
	virtual void MoveSection(float DeltaPosition, TSet<FKeyHandle>& KeyHandles) override;
	virtual void DilateSection(float DilationFactor, float Origin, TSet<FKeyHandle>& KeyHandles) override;
	virtual void GetKeyHandles(TSet<FKeyHandle>& KeyHandles) const override;

	UMG_API FRichCurve& GetTranslationCurve( EAxis::Type Axis );

	UMG_API FRichCurve& GetRotationCurve();

	UMG_API FRichCurve& GetScaleCurve( EAxis::Type Axis );

	UMG_API FRichCurve& GetSheerCurve( EAxis::Type Axis );

	FWidgetTransform Eval( float Position, const FWidgetTransform& DefaultValue ) const;

	bool NewKeyIsNewData( float Time, const FWidgetTransform& Transform, FKeyParams KeyParams ) const;

	void AddKey( float Time, const struct F2DTransformKey& TransformKey, FKeyParams KeyParams );
private:
	/** Translation curves*/
	UPROPERTY()
	FRichCurve Translation[2];
	
	/** Rotation curve */
	UPROPERTY()
	FRichCurve Rotation;

	/** Scale curves */
	UPROPERTY()
	FRichCurve Scale[2];

	/** Shear curve */
	UPROPERTY()
	FRichCurve Shear[2];
};