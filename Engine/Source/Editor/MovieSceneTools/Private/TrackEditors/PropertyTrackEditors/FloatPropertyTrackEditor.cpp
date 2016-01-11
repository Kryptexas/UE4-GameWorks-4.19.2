// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneToolsPrivatePCH.h"
#include "FloatPropertyTrackEditor.h"
#include "FloatPropertySection.h"


TSharedRef<ISequencerTrackEditor> FFloatPropertyTrackEditor::CreateTrackEditor( TSharedRef<ISequencer> OwningSequencer )
{
	return MakeShareable(new FFloatPropertyTrackEditor(OwningSequencer));
}


TSharedRef<ISequencerSection> FFloatPropertyTrackEditor::MakeSectionInterface( UMovieSceneSection& SectionObject, UMovieSceneTrack& Track )
{
	return MakeShareable(new FFloatPropertySection( SectionObject, Track.GetTrackName() ));
}


bool FFloatPropertyTrackEditor::TryGenerateKeyFromPropertyChanged( const UMovieSceneTrack* InTrack, const FPropertyChangedParams& PropertyChangedParams, float& OutKey )
{
	OutKey = *PropertyChangedParams.GetPropertyValue<float>();

	if (InTrack)
	{
		const UMovieSceneFloatTrack* FloatTrack = CastChecked<const UMovieSceneFloatTrack>( InTrack );

		if (FloatTrack)
		{
			float KeyTime =	GetTimeForKey(GetMovieSceneSequence());
			return FloatTrack->CanKeyTrack(KeyTime, OutKey, PropertyChangedParams.KeyParams);
		}
	}

	return false;
}
