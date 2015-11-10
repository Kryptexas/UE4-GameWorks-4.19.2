// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IKeyArea.h"


/**
 * Abstract base class for integral curve key areas.
 */
class MOVIESCENETOOLS_API FIntegralCurveKeyAreaBase
	: public IKeyArea
{
public:

	/** Create and initialize a new instance. */
	FIntegralCurveKeyAreaBase(FIntegralCurve& InCurve, UMovieSceneSection* InOwningSection)
		: Curve(InCurve)
		, OwningSection(InOwningSection)
	{ }

public:

	// IKeyArea interface

	virtual TArray<FKeyHandle> AddKeyUnique( float Time, EMovieSceneKeyInterpolation InKeyInterpolation, float TimeToCopyFrom = FLT_MAX ) override;
	virtual void DeleteKey(FKeyHandle KeyHandle) override;
	virtual ERichCurveExtrapolation GetExtrapolationMode(bool bPreInfinity) const override;
	virtual ERichCurveInterpMode GetKeyInterpMode(FKeyHandle KeyHandle) const override;
	virtual ERichCurveTangentMode GetKeyTangentMode(FKeyHandle KeyHandle) const override;
	virtual float GetKeyTime(FKeyHandle KeyHandle) const override;
	virtual UMovieSceneSection* GetOwningSection() override;
	virtual FRichCurve* GetRichCurve() override;
	virtual TArray<FKeyHandle> GetUnsortedKeyHandles() const override;
	virtual FKeyHandle MoveKey(FKeyHandle KeyHandle, float DeltaPosition) override;
	virtual void SetExtrapolationMode(ERichCurveExtrapolation ExtrapMode, bool bPreInfinity) override;
	virtual void SetKeyInterpMode(FKeyHandle KeyHandle, ERichCurveInterpMode InterpMode) override;
	virtual void SetKeyTangentMode(FKeyHandle KeyHandle, ERichCurveTangentMode TangentMode) override;
	virtual void SetKeyTime(FKeyHandle KeyHandle, float NewKeyTime) const override;

protected:

	virtual void EvaluateAndAddKey(float Time, float TimeToCopyFrom, FKeyHandle& CurrentKey) = 0;

protected:

	/** Curve with keys in this area */
	FIntegralCurve& Curve;

	/** The section that owns this key area. */
	UMovieSceneSection* OwningSection;
};


/**
 * Template for integral curve key areas.
 */
template<typename IntegralType>
class MOVIESCENETOOLS_API FIntegralKeyArea
	: public FIntegralCurveKeyAreaBase
{
public:

	FIntegralKeyArea(FIntegralCurve& InCurve, UMovieSceneSection* InOwningSection)
		: FIntegralCurveKeyAreaBase(InCurve, InOwningSection)
	{ }

public:

	void ClearIntermediateValue()
	{
		IntermediateValue.Reset();
	}

	void SetIntermediateValue(IntegralType InIntermediateValue)
	{
		IntermediateValue = InIntermediateValue;
	}

protected:

	virtual void EvaluateAndAddKey(float Time, float TimeToCopyFrom, FKeyHandle& CurrentKey) override
	{
		int32 Value = Curve.Evaluate(Time);

		if (TimeToCopyFrom != FLT_MAX)
		{
			Value = Curve.Evaluate(TimeToCopyFrom);
		}
		else if (IntermediateValue.IsSet())
		{
			Value = IntermediateValue.GetValue();
		}

		Curve.AddKey(Time, Value, CurrentKey);
	}

protected:

	TOptional<IntegralType> IntermediateValue;
};
