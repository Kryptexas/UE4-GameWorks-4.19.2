// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	CurveFloat.cpp
=============================================================================*/

#include "EnginePrivate.h"

FRuntimeFloatCurve::FRuntimeFloatCurve()
	: ExternalCurve(NULL)
{
}

float FRuntimeFloatCurve::Eval(float InTime) const
{
	if (ExternalCurve)
	{
		return ExternalCurve->GetFloatValue(InTime);
	}
	else
	{
		return EditorCurveData.Eval(InTime);
	}
}

void FRuntimeFloatCurve::GetTimeRange(float& MinTime, float& MaxTime) const
{
	if (ExternalCurve)
	{
		return ExternalCurve->GetTimeRange(MinTime, MaxTime);
	}
	else
	{
		return EditorCurveData.GetTimeRange(MinTime, MaxTime);
	}
}

UCurveFloat::UCurveFloat(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

float UCurveFloat::GetFloatValue( float InTime ) const
{
	return FloatCurve.Eval(InTime);
}

void UCurveFloat::PostLoad()
{
	Super::PostLoad();

	if(GetLinkerUE4Version() < VER_UE4_UCURVE_USING_RICHCURVES)
	{
		FRichCurve::ConvertInterpCurveFloat(FloatKeys_DEPRECATED, FloatCurve);
	}
}

TArray<FRichCurveEditInfoConst> UCurveFloat::GetCurves() const
{
	TArray<FRichCurveEditInfoConst> Curves;
	Curves.Add(FRichCurveEditInfoConst(&FloatCurve));
	return Curves;
}

TArray<FRichCurveEditInfo> UCurveFloat::GetCurves()
{
	TArray<FRichCurveEditInfo> Curves;
	Curves.Add(FRichCurveEditInfo(&FloatCurve));
	return Curves;
}

bool UCurveFloat::operator==( const UCurveFloat& Curve ) const
{
	return bIsEventCurve == Curve.bIsEventCurve && FloatCurve == Curve.FloatCurve;
}

