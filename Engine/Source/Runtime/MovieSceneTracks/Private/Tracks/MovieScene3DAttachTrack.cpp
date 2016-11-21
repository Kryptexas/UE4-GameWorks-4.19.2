// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieScene3DAttachSection.h"
#include "MovieScene3DAttachTrack.h"
#include "IMovieScenePlayer.h"
#include "Evaluation/MovieScene3DAttachTemplate.h"
#include "Evaluation/MovieSceneEvaluationTrack.h"


#define LOCTEXT_NAMESPACE "MovieScene3DAttachTrack"


UMovieScene3DAttachTrack::UMovieScene3DAttachTrack( const FObjectInitializer& ObjectInitializer )
	: Super( ObjectInitializer )
{ }

FMovieSceneEvalTemplatePtr UMovieScene3DAttachTrack::CreateTemplateForSection(const UMovieSceneSection& InSection) const
{
	return FMovieScene3DAttachSectionTemplate(*CastChecked<UMovieScene3DAttachSection>(&InSection));
}

void UMovieScene3DAttachTrack::AddConstraint(float KeyTime, float ConstraintEndTime, const FName SocketName, const FName ComponentName, const FGuid& ConstraintId)
{
	// add the section
	UMovieScene3DAttachSection* NewSection = NewObject<UMovieScene3DAttachSection>(this);
	NewSection->SetFlags(RF_Transactional);
	NewSection->AddAttach(KeyTime, ConstraintEndTime, ConstraintId);
	NewSection->InitialPlacement( ConstraintSections, KeyTime, ConstraintEndTime, SupportsMultipleRows() );
	NewSection->AttachSocketName = SocketName;
	NewSection->AttachComponentName = ComponentName;

	ConstraintSections.Add(NewSection);
}

UMovieSceneSection* UMovieScene3DAttachTrack::CreateNewSection()
{
	UMovieScene3DAttachSection* NewSection = NewObject<UMovieScene3DAttachSection>(this);
	NewSection->SetFlags(RF_Transactional);

	ConstraintSections.Add(NewSection);

	return NewSection;
}

#if WITH_EDITORONLY_DATA
FText UMovieScene3DAttachTrack::GetDisplayName() const
{
	return LOCTEXT("TrackName", "Attach");
}
#endif


#undef LOCTEXT_NAMESPACE
