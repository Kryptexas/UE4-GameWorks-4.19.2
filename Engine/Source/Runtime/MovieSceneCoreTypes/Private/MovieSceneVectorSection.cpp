// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneCoreTypesPCH.h"
#include "MovieSceneVectorSection.h"
#include "MovieSceneVectorTrack.h"

UMovieSceneVectorSection::UMovieSceneVectorSection( const FPostConstructInitializeProperties& PCIP )
	: Super( PCIP )
{

}

FVector4 UMovieSceneVectorSection::Eval( float Position ) const
{
	return FVector4(
		Curves[0].Eval( Position ),
		Curves[1].Eval( Position ),
		Curves[2].Eval( Position ),
		Curves[3].Eval( Position ) );
}

void UMovieSceneVectorSection::AddKey( float Time, const FVector4& Value )
{
	check(ChannelsUsed >= 2 && ChannelsUsed <= 4);

	for (int32 i = 0; i < ChannelsUsed; ++i)
	{
		Curves[i].UpdateOrAddKey(Time, Value[i]);
	}
}

bool UMovieSceneVectorSection::NewKeyIsNewData(float Time, const FVector4& Value) const
{
	check(ChannelsUsed >= 2 && ChannelsUsed <= 4);

	bool bNewData = false;
	for (int32 i = 0; i < ChannelsUsed; ++i)
	{
		float OriginalData = Curves[i].Eval(Time);
		// don't re-add keys if the data already matches
		if (Curves[i].GetNumKeys() == 0 || !FMath::IsNearlyEqual(OriginalData, Value[i]))
		{
			bNewData = true;
			break;
		}
	}
	return bNewData;
}

void UMovieSceneVectorSection::MoveSection( float DeltaTime )
{
	check(ChannelsUsed >= 2 && ChannelsUsed <= 4);

	Super::MoveSection( DeltaTime );

	for (int32 i = 0; i < ChannelsUsed; ++i)
	{
		Curves[i].ShiftCurve(DeltaTime);
	}
}

void UMovieSceneVectorSection::DilateSection( float DilationFactor, float Origin )
{
	check(ChannelsUsed >= 2 && ChannelsUsed <= 4);

	for (int32 i = 0; i < ChannelsUsed; ++i)
	{
		Curves[i].ScaleCurve(Origin, DilationFactor);
	}

	Super::DilateSection(DilationFactor, Origin);
}
