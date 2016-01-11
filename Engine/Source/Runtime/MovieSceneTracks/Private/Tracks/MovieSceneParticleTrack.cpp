// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieSceneParticleSection.h"
#include "MovieSceneParticleTrack.h"
#include "IMovieScenePlayer.h"
#include "MovieSceneParticleTrackInstance.h"


#define LOCTEXT_NAMESPACE "MovieSceneParticleTrack"


UMovieSceneParticleTrack::UMovieSceneParticleTrack( const FObjectInitializer& ObjectInitializer )
	: Super( ObjectInitializer )
{ }


FName UMovieSceneParticleTrack::GetTrackName() const
{
	return FName("ParticleSystem");
}


TSharedPtr<IMovieSceneTrackInstance> UMovieSceneParticleTrack::CreateInstance()
{
	return MakeShareable( new FMovieSceneParticleTrackInstance( *this ) ); 
}


const TArray<UMovieSceneSection*>& UMovieSceneParticleTrack::GetAllSections() const
{
	return ParticleSections;
}


void UMovieSceneParticleTrack::RemoveAllAnimationData()
{
	// do nothing
}


bool UMovieSceneParticleTrack::HasSection( UMovieSceneSection* Section ) const
{
	return ParticleSections.Find( Section ) != INDEX_NONE;
}


void UMovieSceneParticleTrack::AddSection( UMovieSceneSection* Section )
{
	ParticleSections.Add( Section );
}


void UMovieSceneParticleTrack::RemoveSection( UMovieSceneSection* Section )
{
	ParticleSections.Remove( Section );
}


bool UMovieSceneParticleTrack::IsEmpty() const
{
	return ParticleSections.Num() == 0;
}


TRange<float> UMovieSceneParticleTrack::GetSectionBoundaries() const
{
	TArray< TRange<float> > Bounds;
	for (int32 i = 0; i < ParticleSections.Num(); ++i)
	{
		Bounds.Add(ParticleSections[i]->GetRange());
	}
	return TRange<float>::Hull(Bounds);
}

void UMovieSceneParticleTrack::AddNewKey( float KeyTime )
{
	UMovieSceneParticleSection* NearestSection = Cast<UMovieSceneParticleSection>( MovieSceneHelpers::FindNearestSectionAtTime( ParticleSections, KeyTime ) );
	if ( NearestSection == nullptr )
	{
		NearestSection = NewObject<UMovieSceneParticleSection>( this );
		NearestSection->SetStartTime( KeyTime );
		NearestSection->SetEndTime( KeyTime );
		ParticleSections.Add(NearestSection);
	}
	else
	{
		if ( NearestSection->GetStartTime() > KeyTime )
		{
			NearestSection->SetStartTime( KeyTime );
		}
		if ( NearestSection->GetEndTime() < KeyTime )
		{
			NearestSection->SetEndTime( KeyTime );
		}
	}
	NearestSection->AddKey( KeyTime, EParticleKey::Active );
}


#undef LOCTEXT_NAMESPACE
