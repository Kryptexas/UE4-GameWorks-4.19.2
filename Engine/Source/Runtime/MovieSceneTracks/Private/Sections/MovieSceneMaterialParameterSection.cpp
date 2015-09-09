// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieSceneMaterialParameterSection.h"

FScalarParameterNameAndCurve::FScalarParameterNameAndCurve( FName InParameterName )
{
	ParameterName = InParameterName;
}

UMovieSceneMaterialParameterSection::UMovieSceneMaterialParameterSection( const FObjectInitializer& ObjectInitializer )
	: Super( ObjectInitializer )
{
}

void UMovieSceneMaterialParameterSection::AddScalarParameterKey( FName InParameterName, float InTime, float InValue )
{
	FRichCurve* ExistingCurve = GetScalarParameterCurve(InParameterName);
	if ( ExistingCurve != nullptr )
	{
		ExistingCurve->AddKey(InTime, InValue);
	}
	else
	{
		int32 NewIndex = ScalarParameterNamesAndCurves.Add( FScalarParameterNameAndCurve() );
		ScalarParameterNamesAndCurves[NewIndex].ParameterName = InParameterName;
		ScalarParameterNamesAndCurves[NewIndex].ParameterCurve.AddKey( InTime, InValue );
	}
}

TArray<FScalarParameterNameAndCurve>* UMovieSceneMaterialParameterSection::GetScalarParameterNamesAndCurves()
{
	return &ScalarParameterNamesAndCurves;
}

FRichCurve* UMovieSceneMaterialParameterSection::GetScalarParameterCurve( FName InParameterName )
{
	for ( FScalarParameterNameAndCurve& ScalarParameterNameAndCurve : ScalarParameterNamesAndCurves )
	{
		if ( ScalarParameterNameAndCurve.ParameterName == InParameterName )
		{
			return &ScalarParameterNameAndCurve.ParameterCurve;
		}
	}
	return nullptr;
}