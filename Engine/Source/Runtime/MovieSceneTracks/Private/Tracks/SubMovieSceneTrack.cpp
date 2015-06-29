// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "SubMovieSceneTrack.h"
#include "IMovieScenePlayer.h"
#include "SubMovieSceneSection.h"
#include "SubMovieSceneTrackInstance.h"

USubMovieSceneTrack::USubMovieSceneTrack( const FObjectInitializer& ObjectInitializer )
	: Super( ObjectInitializer )
{
}

UMovieSceneSection* USubMovieSceneTrack::CreateNewSection()
{
	return NewObject<USubMovieSceneSection>(this, NAME_None, RF_Transactional);
}

FName USubMovieSceneTrack::GetTrackName() const
{
	static const FName DefaultName( "MovieScene" );

	FName Name = DefaultName;
	if( SubMovieSceneSections.Num() > 0 )
	{
		Name = CastChecked<USubMovieSceneSection>(SubMovieSceneSections[0])->GetMovieScene()->GetFName();
	}

	return Name;
}

TSharedPtr<IMovieSceneTrackInstance> USubMovieSceneTrack::CreateInstance()
{
	return MakeShareable( new FSubMovieSceneTrackInstance( *this ) ); 
}

const TArray<UMovieSceneSection*>& USubMovieSceneTrack::GetAllSections() const 
{
	return SubMovieSceneSections;
}

void USubMovieSceneTrack::RemoveSection( UMovieSceneSection* Section )
{
	USubMovieSceneSection* SubMovieSceneSection = Cast<USubMovieSceneSection>( Section );
	if(SubMovieSceneSection)
	{
		SubMovieSceneSections.Remove( SubMovieSceneSection );
	}
}

void USubMovieSceneTrack::RemoveAllAnimationData()
{
	SubMovieSceneSections.Empty();
}

bool USubMovieSceneTrack::IsEmpty() const
{
	return SubMovieSceneSections.Num() == 0;
}

TRange<float> USubMovieSceneTrack::GetSectionBoundaries() const
{
	TArray< TRange<float> > Bounds;
	for (int32 SectionIndex = 0; SectionIndex < SubMovieSceneSections.Num(); ++SectionIndex)
	{
		Bounds.Add(SubMovieSceneSections[SectionIndex]->GetRange());
	}
	return TRange<float>::Hull(Bounds);
}

bool USubMovieSceneTrack::HasSection( UMovieSceneSection* Section ) const
{
	return SubMovieSceneSections.Contains( Section );
}

void USubMovieSceneTrack::AddMovieSceneSection( UMovieScene* SubMovieScene, float Time )
{
	Modify();

	USubMovieSceneSection* NewSection = CastChecked<USubMovieSceneSection>( CreateNewSection() );

	NewSection->SetMovieScene( SubMovieScene );
	NewSection->SetStartTime( Time );
	NewSection->SetEndTime( Time + SubMovieScene->GetTimeRange().Size<float>() );

	SubMovieSceneSections.Add( NewSection );

}
