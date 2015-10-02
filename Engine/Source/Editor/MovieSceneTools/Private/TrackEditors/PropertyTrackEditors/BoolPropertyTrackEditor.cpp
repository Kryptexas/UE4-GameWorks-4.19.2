// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneToolsPrivatePCH.h"
#include "MovieSceneBoolTrack.h"
#include "BoolPropertyTrackEditor.h"
#include "BoolPropertySection.h"


TSharedRef<ISequencerTrackEditor> FBoolPropertyTrackEditor::CreateTrackEditor( TSharedRef<ISequencer> OwningSequencer )
{
	return MakeShareable(new FBoolPropertyTrackEditor(OwningSequencer));
}


TSharedRef<ISequencerSection> FBoolPropertyTrackEditor::MakeSectionInterface( UMovieSceneSection& SectionObject, UMovieSceneTrack& Track )
{
	return MakeShareable(new FBoolPropertySection( SectionObject, Track.GetTrackName(), GetSequencer().Get() ));
}


bool FBoolPropertyTrackEditor::TryGenerateKeyFromPropertyChanged( const UMovieSceneTrack* InTrack, const FPropertyChangedParams& PropertyChangedParams, bool& OutKey )
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
