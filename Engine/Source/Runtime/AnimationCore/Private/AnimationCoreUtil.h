// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AnimationCoreUtil.h: Render core module definitions.
=============================================================================*/

#pragma once

#include "CoreMinimal.h"

struct FComponentBlendHelper
{
	void Reset()
	{
		Translations.Reset();
		Rotations.Reset();
		Scales.Reset();
		TranslationWeights.Reset();
		RotationWeights.Reset();
		ScaleWeights.Reset();
	}

	// @lamda?
	void AddTranslation(const FVector& Translation, float Weight)
	{
		Translations.Add(Translation);
		TranslationWeights.Add(Weight);
		ensureAlways(Translations.Num() == TranslationWeights.Num());
	}

	void AddRotation(const FQuat& Rotation, float Weight)
	{
		Rotations.Add(Rotation);
		RotationWeights.Add(Weight);
		ensureAlways(Rotations.Num() == RotationWeights.Num());
	}

	void AddScale(const FVector& Scale, float Weight)
	{
		Scales.Add(Scale);
		ScaleWeights.Add(Weight);
		ensureAlways(Scales.Num() == ScaleWeights.Num());
	}

	bool GetBlendedTranslation(FVector& Output)
	{
		// there is no correct value to return if no translation
		// so if false, do not use this value
		if (Translations.Num() > 0)
		{
			float TotalWeight = GetTotalWeight(TranslationWeights);

			if (TotalWeight > ZERO_ANIMWEIGHT_THRESH)
			{
				float MultiplyWeight = (TotalWeight > 1.f) ? 1.f / TotalWeight : 1.f;

				Output = Translations[0] * (TranslationWeights[0] * MultiplyWeight);

				for (int32 Index = 1; Index < Translations.Num(); ++Index)
				{
					Output += Translations[Index] * (TranslationWeights[Index] * MultiplyWeight);
				}

				return true;
			}
		}

		return false;
	}

	bool GetBlendedRotation(FQuat& Output)
	{
		// there is no correct value to return if no translation
		// so if false, do not use this value
		if (Rotations.Num() > 0)
		{
			float TotalWeight = GetTotalWeight(RotationWeights);

			if (TotalWeight > ZERO_ANIMWEIGHT_THRESH)
			{
				float MultiplyWeight = (TotalWeight > 1.f) ? 1.f / TotalWeight : 1.f;

				Output = Rotations[0] * (RotationWeights[0] * MultiplyWeight);

				for (int32 Index = 1; Index < Rotations.Num(); ++Index)
				{
					FQuat BlendRotation = Rotations[Index] * (RotationWeights[Index] * MultiplyWeight);
					if ((Output | BlendRotation) < 0)
					{
						Output.X -= BlendRotation.X;
						Output.Y -= BlendRotation.Y;
						Output.Z -= BlendRotation.Z;
						Output.W -= BlendRotation.W;
					}
					else
					{
						Output.X += BlendRotation.X;
						Output.Y += BlendRotation.Y;
						Output.Z += BlendRotation.Z;
						Output.W += BlendRotation.W;
					}
				}

				Output.Normalize();
				return true;
			}
		}

		return false;
	}
	bool GetBlendedScale(FVector& Output)
	{
		// there is no correct value to return if no translation
		// so if false, do not use this value
		if (Scales.Num() > 0)
		{
			float TotalWeight = GetTotalWeight(ScaleWeights);

			if (TotalWeight > ZERO_ANIMWEIGHT_THRESH)
			{
				float MultiplyWeight = (TotalWeight > 1.f) ? 1.f / TotalWeight : 1.f;

				Output = Scales[0] * (ScaleWeights[0] * MultiplyWeight);

				for (int32 Index = 1; Index < Scales.Num(); ++Index)
				{
					Output *= Scales[Index] * (ScaleWeights[Index] * MultiplyWeight);
				}

				return true;
			}
		}

		return false;
	}

private:
	// data member to accumulate blending intermediate result per component
	TArray<FVector>		Translations;
	TArray<FQuat>		Rotations;
	TArray<FVector>		Scales;
	TArray<float>		TranslationWeights;
	TArray<float>		RotationWeights;
	TArray<float>		ScaleWeights;

	float GetTotalWeight(const TArray<float>& Weights)
	{
		float TotalWeight = 0.f;
		for (float Weight : Weights)
		{
			TotalWeight += Weight;
		}

		return TotalWeight;
	}
};