// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneToolsPrivatePCH.h"
#include "MovieSceneBoolTrack.h"
#include "BoolPropertyTrackEditor.h"
#include "BoolPropertySection.h"

TSharedRef<FMovieSceneTrackEditor> FBoolPropertyTrackEditor::CreateTrackEditor( TSharedRef<ISequencer> OwningSequencer )
{
	return MakeShareable(new FBoolPropertyTrackEditor(OwningSequencer));
}

TSharedRef<ISequencerSection> FBoolPropertyTrackEditor::MakeSectionInterface( UMovieSceneSection& SectionObject, UMovieSceneTrack* Track )
{
	return MakeShareable(new FBoolPropertySection( SectionObject, Track->GetTrackName(), GetSequencer().Get() ));
}

bool FBoolPropertyTrackEditor::TryGenerateKeyFromPropertyChanged( const UMovieSceneTrack* InTrack, const FPropertyChangedParams& PropertyChangedParams, bool& OutKey )
{
	const UBoolProperty* BoolProperty = Cast<const UBoolProperty>(PropertyChangedParams.PropertyPath.Last());
	if (BoolProperty)
	{
		OutKey = BoolProperty->GetPropertyValue(BoolProperty->ContainerPtrToValuePtr<void>(PropertyChangedParams.ObjectsThatChanged.Last()));
	
		if (InTrack)
		{
			const UMovieSceneBoolTrack* BoolTrack = CastChecked<const UMovieSceneBoolTrack>( InTrack );
			if (BoolTrack)
			{
				float KeyTime =	GetTimeForKey(GetMovieSceneSequence());
				return BoolTrack->CanKeyTrack(KeyTime, OutKey, PropertyChangedParams.KeyParams);
			}
		}
	}

	return false;
}