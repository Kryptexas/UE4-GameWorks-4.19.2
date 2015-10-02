// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneToolsPrivatePCH.h"
#include "MovieSceneVisibilityTrack.h"
#include "VisibilityPropertyTrackEditor.h"
#include "VisibilityPropertySection.h"


TSharedRef<ISequencerTrackEditor> FVisibilityPropertyTrackEditor::CreateTrackEditor( TSharedRef<ISequencer> OwningSequencer )
{
	return MakeShareable(new FVisibilityPropertyTrackEditor(OwningSequencer));
}


TSharedRef<ISequencerSection> FVisibilityPropertyTrackEditor::MakeSectionInterface( UMovieSceneSection& SectionObject, UMovieSceneTrack& Track )
{
	return MakeShareable(new FVisibilityPropertySection( SectionObject, Track.GetTrackName(), GetSequencer().Get() ));
}


bool FVisibilityPropertyTrackEditor::TryGenerateKeyFromPropertyChanged( const UMovieSceneTrack* InTrack, const FPropertyChangedParams& PropertyChangedParams, bool& OutKey )
{
	const UBoolProperty* BoolProperty = Cast<const UBoolProperty>(PropertyChangedParams.PropertyPath.Last());

	if (BoolProperty && PropertyChangedParams.ObjectsThatChanged.Num() != 0)
	{
		void* CurrentObject = PropertyChangedParams.ObjectsThatChanged[0];
		for (int32 i = 0; i < PropertyChangedParams.PropertyPath.Num(); ++i)
		{
			CurrentObject = PropertyChangedParams.PropertyPath[i]->ContainerPtrToValuePtr<void>(CurrentObject, 0);
		}
		OutKey = BoolProperty->GetPropertyValue(CurrentObject);

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
