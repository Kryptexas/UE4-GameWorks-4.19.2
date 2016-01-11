// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneToolsPrivatePCH.h"
#include "MovieSceneVectorTrack.h"
#include "VectorPropertyTrackEditor.h"
#include "VectorPropertySection.h"


TSharedRef<ISequencerTrackEditor> FVectorPropertyTrackEditor::CreateTrackEditor( TSharedRef<ISequencer> InSequencer )
{
	return MakeShareable( new FVectorPropertyTrackEditor( InSequencer ) );
}


TSharedRef<ISequencerSection> FVectorPropertyTrackEditor::MakeSectionInterface( UMovieSceneSection& SectionObject, UMovieSceneTrack& Track )
{
	return MakeShareable(new FVectorPropertySection( SectionObject, Track.GetTrackName() ));
}


bool FVectorPropertyTrackEditor::TryGenerateKeyFromPropertyChanged( const UMovieSceneTrack* InTrack, const FPropertyChangedParams& PropertyChangedParams, FVectorKey& OutKey )
{
	bool bIsVector2D = false, bIsVector = false, bIsVector4 = false;
	const UStructProperty* StructProp = Cast<const UStructProperty>( PropertyChangedParams.PropertyPath.Last() );

	if ( StructProp && StructProp->Struct )
	{
		FName StructName = StructProp->Struct->GetFName();

		bIsVector2D = StructName == NAME_Vector2D;
		bIsVector = StructName == NAME_Vector;
		bIsVector4 = StructName == NAME_Vector4;
	}

	if ( !bIsVector2D && !bIsVector && !bIsVector4 )
	{
		return false;
	}

	// Get the vector value from the property
	if ( bIsVector2D )
	{
		OutKey = FVectorKey(*PropertyChangedParams.GetPropertyValue<FVector2D>(), PropertyChangedParams.StructPropertyNameToKey );
	}
	else if ( bIsVector )
	{
		OutKey = FVectorKey( *PropertyChangedParams.GetPropertyValue<FVector>(), PropertyChangedParams.StructPropertyNameToKey );
	}
	else // if ( bIsVector4 )
	{
		OutKey = FVectorKey( *PropertyChangedParams.GetPropertyValue<FVector4>(), PropertyChangedParams.StructPropertyNameToKey );
	}

	if (InTrack)
	{
		const UMovieSceneVectorTrack* VectorTrack = CastChecked<const UMovieSceneVectorTrack>( InTrack );
		if (VectorTrack)
		{
			float KeyTime =	GetTimeForKey(GetMovieSceneSequence());
			return VectorTrack->CanKeyTrack(KeyTime, OutKey, PropertyChangedParams.KeyParams);
		}
	}

	return false;
}
