// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Curves/RichCurve.h"
#include "Evaluation/MovieSceneEvalTemplate.h"
#include "MovieScene3DTransformSection.h"
#include "Blending/BlendableToken.h"
#include "MovieScene3DTransformTemplate.generated.h"

class UMovieScene3DTransformSection;

namespace MovieScene
{
	/** A deconstructed 3-dimensional transform used for blending */
	struct FSimpleTransform
	{
		/** The translation, rotation and scale components */
		FVector Translation, Rotation, Scale;

		FSimpleTransform();
		FSimpleTransform(FVector InTranslation, FVector InRotation, FVector InScale);

		/** Add another transform onto this one */
		FSimpleTransform& operator+=(const FSimpleTransform& Other);

		/** Mathematical operators */
		friend FSimpleTransform operator+(const FSimpleTransform& A, const FSimpleTransform& B);
		friend FSimpleTransform operator*(const FSimpleTransform& A, const FSimpleTransform& B);
		friend FSimpleTransform operator/(const FSimpleTransform& A, const FSimpleTransform& B);
		friend FSimpleTransform operator*(const FSimpleTransform& In, float InScalar);
	};

	/**
	 * Generate a mask transform from a channel mask.
	 * @return a transform with either 1 or 0 in the channels that are set by the specified mask
	 */
	FSimpleTransform GetMaskTransform(FMovieSceneTransformMask Mask);

	/**
	 * Generate a channel mask from a transform.
	 * @return A channel mask with bits set for non-zero channels from the specified transform
	 */
	EMovieSceneTransformChannel GetMaskFromTransform(const FSimpleTransform& Transform);

	/** A simple transform that has channels masked out */
	struct FMaskedTransform : FSimpleTransform
	{
		/** The mask that defines which channels are valid in this transform */
		FMovieSceneTransformMask Mask;

		FMaskedTransform(FMovieSceneTransformMask InMask) : Mask(InMask) {}
	};

	/** Working data type for blending of transforms */
	struct FBlendableTransform
	{
		/** The cumulative total absolute and relative transform, scaled by weight */
		FSimpleTransform Absolute;
		/** The channel-specific weightings that are applied to the above transform */
		FSimpleTransform AbsoluteWeight;
		/** A mask specifying which channels have been animated in the above transform */
		EMovieSceneTransformChannel AbsoluteMask;

		/** Cumulative additive total, scaled by weight */
		FSimpleTransform Additive;
		/** A mask specifying which channels have been animated in the above transform */
		EMovieSceneTransformChannel AdditiveMask;

		/** Resolve this structure's data into a final transform to apply to the object */
		FSimpleTransform Resolve(TMovieSceneInitialValueStore<FSimpleTransform>& InitialValueStore);
	};

	/**
	 * Blend function used to blend a masked transform into the specified working transform
	 */
	void BlendValue(FBlendableTransform& OutBlend, const FMaskedTransform& InValue, float Weight, EMovieSceneBlendType BlendType, TMovieSceneInitialValueStore<FSimpleTransform>& InitialValueStore);
}

/** Cause FBlendableTransform to be used to accumulate tokens of type FSimpleTransform */
template<> struct TBlendableTokenTraits<MovieScene::FSimpleTransform>
{
	typedef MovieScene::FBlendableTransform WorkingDataType;
};

/** Define a blending data type identifier for FSimpleTransforms. Exported to ensure that only 1 unique ID is instantiated for FSimpleTransform. */
template<> MOVIESCENETRACKS_API FMovieSceneAnimTypeID GetBlendingDataType<MovieScene::FSimpleTransform>();

USTRUCT()
struct FMovieScene3DTransformTemplateData
{
	GENERATED_BODY()

	FMovieScene3DTransformTemplateData(){}
	FMovieScene3DTransformTemplateData(const UMovieScene3DTransformSection& Section);

	MovieScene::FMaskedTransform Evaluate(float InTime) const;

	UPROPERTY()
	FRichCurve TranslationCurve[3];

	UPROPERTY()
	FRichCurve RotationCurve[3];

	UPROPERTY()
	FRichCurve ScaleCurve[3];

	UPROPERTY()
	FRichCurve ManualWeight;

	UPROPERTY() 
	EMovieSceneBlendType BlendType;

	UPROPERTY()
	FMovieSceneTransformMask Mask;
};

USTRUCT()
struct FMovieSceneComponentTransformSectionTemplate : public FMovieSceneEvalTemplate
{
	GENERATED_BODY()

	UPROPERTY()
	FMovieScene3DTransformTemplateData TemplateData;

	FMovieSceneComponentTransformSectionTemplate(){}
	FMovieSceneComponentTransformSectionTemplate(const UMovieScene3DTransformSection& Section);

protected:

	virtual UScriptStruct& GetScriptStructImpl() const override { return *StaticStruct(); }
	virtual void Evaluate(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const override;
	virtual void Interrogate(const FMovieSceneContext& Context, FMovieSceneInterrogationData& Container, UObject* BindingOverride) const override;
};
