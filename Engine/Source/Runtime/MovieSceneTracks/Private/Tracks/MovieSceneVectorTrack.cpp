// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieSceneVectorSection.h"
#include "MovieSceneVectorTrack.h"
#include "IMovieScenePlayer.h"
#include "MovieSceneVectorTrackInstance.h"

FVectorKey::FVectorKey( FVector2D InValue, FName InCurveName, bool InbAddKeyEvenIfUnChanged )
{
	Value.X = InValue.X;
	Value.Y = InValue.Y;
	Value.Z = 0;
	Value.W = 0;
	ChannelsUsed = 2;
	CurveName = InCurveName;
	bAddKeyEvenIfUnchanged = InbAddKeyEvenIfUnChanged;
}

FVectorKey::FVectorKey( FVector InValue, FName InCurveName, bool InbAddKeyEvenIfUnChanged )
{
	Value.X = InValue.X;
	Value.Y = InValue.Y;
	Value.Z = InValue.Z;
	Value.W = 0;
	ChannelsUsed = 3;
	CurveName = InCurveName;
	bAddKeyEvenIfUnchanged = InbAddKeyEvenIfUnChanged;
}

FVectorKey::FVectorKey( FVector4 InValue, FName InCurveName, bool InbAddKeyEvenIfUnChanged )
{
	Value = InValue;
	ChannelsUsed = 4;
	CurveName = InCurveName;
	bAddKeyEvenIfUnchanged = InbAddKeyEvenIfUnChanged;
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

bool UMovieSceneVectorTrack::AddKeyToSection( float Time, const FVector4& Value, int32 InChannelsUsed, FName CurveName, bool bAddKeyEvenIfUnchanged )
{
	const UMovieSceneSection* NearestSection = MovieSceneHelpers::FindNearestSectionAtTime( Sections, Time );
	if (!NearestSection || bAddKeyEvenIfUnchanged || CastChecked<UMovieSceneVectorSection>(NearestSection)->NewKeyIsNewData(Time, Value))
	{
		Modify();

		UMovieSceneVectorSection* NewSection = Cast<UMovieSceneVectorSection>( FindOrAddSection( Time ) );
		// @todo Sequencer - I don't like setting the channels used here. It should only be checked here, and set on section creation
		NewSection->SetChannelsUsed(InChannelsUsed);

		NewSection->AddKey( Time, CurveName, Value );

		// We dont support one track containing multiple sections of differing channel amounts
		NumChannelsUsed = InChannelsUsed;

		return true;
	}
	return false;
}

bool UMovieSceneVectorTrack::AddKeyToSection( float Time, FVectorKey Key )
{
	return AddKeyToSection(Time, Key.Value, Key.ChannelsUsed, Key.CurveName, Key.bAddKeyEvenIfUnchanged );
}

bool UMovieSceneVectorTrack::Eval( float Position, float LastPosition, FVector4& InOutVector ) const
{
	const UMovieSceneSection* Section = MovieSceneHelpers::FindSectionAtTime( Sections, Position );

	if( Section )
	{
		InOutVector = CastChecked<UMovieSceneVectorSection>( Section )->Eval( Position, InOutVector );
	}

	return Section != NULL;
}

