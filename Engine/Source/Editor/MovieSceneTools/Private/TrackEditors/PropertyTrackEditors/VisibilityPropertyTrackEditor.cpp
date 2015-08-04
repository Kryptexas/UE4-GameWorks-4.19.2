// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneToolsPrivatePCH.h"
#include "VisibilityPropertyTrackEditor.h"
#include "VisibilityPropertySection.h"

TSharedRef<FMovieSceneTrackEditor> FVisibilityPropertyTrackEditor::CreateTrackEditor( TSharedRef<ISequencer> OwningSequencer )
{
	return MakeShareable(new FVisibilityPropertyTrackEditor(OwningSequencer));
}

TSharedRef<ISequencerSection> FVisibilityPropertyTrackEditor::MakeSectionInterface( UMovieSceneSection& SectionObject, UMovieSceneTrack* Track )
{
	return MakeShareable(new FVisibilityPropertySection( SectionObject, Track->GetTrackName(), GetSequencer().Get() ));
}

bool FVisibilityPropertyTrackEditor::TryGenerateKeyFromPropertyChanged( const FPropertyChangedParams& PropertyChangedParams, bool& OutKey )
{
	const UBoolProperty* BoolProperty = Cast<const UBoolProperty>(PropertyChangedParams.PropertyPath.Last());
	if (BoolProperty)
	{
		OutKey = BoolProperty->GetPropertyValue(BoolProperty->ContainerPtrToValuePtr<void>(PropertyChangedParams.ObjectsThatChanged.Last()));
		return true;
	}

	return false;
}