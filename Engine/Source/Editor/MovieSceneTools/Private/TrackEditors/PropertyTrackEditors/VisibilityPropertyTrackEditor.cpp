// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneToolsPrivatePCH.h"
#include "MovieSceneVisibilityTrack.h"
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

bool FVisibilityPropertyTrackEditor::TryGenerateKeyFromPropertyChanged( const UMovieSceneTrack* InTrack, const FPropertyChangedParams& PropertyChangedParams, bool& OutKey )
{
	const UBoolProperty* BoolProperty = Cast<const UBoolProperty>(PropertyChangedParams.PropertyPath.Last());
	if (BoolProperty)
	{
		OutKey = BoolProperty->GetPropertyValue(BoolProperty->ContainerPtrToValuePtr<void>(PropertyChangedParams.ObjectsThatChanged.Last()));

		if (InTrack)
		{
			const UMovieSceneVisibilityTrack* VisibilityTrack = CastChecked<const UMovieSceneVisibilityTrack>( InTrack );
			if (VisibilityTrack)
			{
				float KeyTime =	GetTimeForKey(GetMovieSceneSequence());
				return VisibilityTrack->CanKeyTrack(KeyTime, OutKey, PropertyChangedParams.KeyParams);
			}
		}
	}

	return false;
}