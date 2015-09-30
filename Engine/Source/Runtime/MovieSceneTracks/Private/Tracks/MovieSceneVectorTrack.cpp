// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieSceneVectorSection.h"
#include "MovieSceneVectorTrack.h"
#include "IMovieScenePlayer.h"
#include "MovieSceneVectorTrackInstance.h"


FVectorKey::FVectorKey( const FVector2D& InValue, FName InCurveName )
{
	Value.X = InValue.X;
	Value.Y = InValue.Y;
	Value.Z = 0;
	Value.W = 0;
	ChannelsUsed = 2;
	CurveName = InCurveName;
}


FVectorKey::FVectorKey( const FVector& InValue, FName InCurveName )
{
	Value.X = InValue.X;
	Value.Y = InValue.Y;
	Value.Z = InValue.Z;
	Value.W = 0;
	ChannelsUsed = 3;
	CurveName = InCurveName;
}


FVectorKey::FVectorKey( const FVector4& InValue, FName InCurveName )
{
	Value = InValue;
	ChannelsUsed = 4;
	CurveName = InCurveName;
}


UMovieSceneVectorTrack::UMovieSceneVectorTrack( const FObjectInitializer& ObjectInitializer )
	: Super( ObjectInitializer )
{
	NumChannelsUsed = 0;
}


UMovieSceneSection* UMovieSceneVectorTrack::CreateNewSection()
{
	return NewObject<UMovieSceneSection>(this, UMovieSceneVectorSection::StaticClass());
}


TSharedPtr<IMovieSceneTrackInstance> UMovieSceneVectorTrack::CreateInstance()
{
	return MakeShareable( new FMovieSceneVectorTrackInstance( *this ) );
}


bool UMovieSceneVectorTrack::AddKeyToSection( float Time, const FVector4& Value, int32 InChannelsUsed, FName CurveName, FKeyParams KeyParams )
{
	const UMovieSceneSection* NearestSection = MovieSceneHelpers::FindNearestSectionAtTime( Sections, Time );
	if (!NearestSection || KeyParams.bAddKeyEvenIfUnchanged || CastChecked<UMovieSceneVectorSection>(NearestSection)->NewKeyIsNewData(Time, Value, KeyParams))
	{
		Modify();

		UMovieSceneVectorSection* NewSection = Cast<UMovieSceneVectorSection>( FindOrAddSection( Time ) );
		// @todo Sequencer - I don't like setting the channels used here. It should only be checked here, and set on section creation
		NewSection->SetChannelsUsed(InChannelsUsed);

		NewSection->AddKey( Time, CurveName, Value, KeyParams );

		// We dont support one track containing multiple sections of differing channel amounts
		NumChannelsUsed = InChannelsUsed;

		return true;
	}

	return false;
}


bool UMovieSceneVectorTrack::AddKeyToSection( float Time, const FVectorKey& Key, FKeyParams KeyParams )
{
	return AddKeyToSection(Time, Key.Value, Key.ChannelsUsed, Key.CurveName, KeyParams );
}


bool UMovieSceneVectorTrack::Eval( float Position, float LastPosition, FVector4& InOutVector ) const
{
	const UMovieSceneSection* Section = MovieSceneHelpers::FindNearestSectionAtTime( Sections, Position );

	if( Section )
	{
		if (!Section->IsInfinite())
		{
			Position = FMath::Clamp(Position, Section->GetStartTime(), Section->GetEndTime());
		}

		InOutVector = CastChecked<UMovieSceneVectorSection>( Section )->Eval( Position, InOutVector );
	}

	return Section != nullptr;
}

bool UMovieSceneVectorTrack::CanKeyTrack(float Time, const FVectorKey& Key, FKeyParams KeyParams) const
{
	const UMovieSceneSection* NearestSection = MovieSceneHelpers::FindNearestSectionAtTime( Sections, Time );
	if (!NearestSection || CastChecked<UMovieSceneVectorSection>(NearestSection)->NewKeyIsNewData(Time, Key.Value, KeyParams))
	{
		return true;
	}
	return false;
}

