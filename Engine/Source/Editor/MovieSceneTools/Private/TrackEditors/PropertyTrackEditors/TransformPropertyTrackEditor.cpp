// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "TrackEditors/PropertyTrackEditors/TransformPropertyTrackEditor.h"
#include "MatineeImportTools.h"
#include "UnrealEdGlobals.h"
#include "Classes/Editor/UnrealEdEngine.h"
#include "Sections/TransformPropertySection.h"

TSharedRef<ISequencerTrackEditor> FTransformPropertyTrackEditor::CreateTrackEditor( TSharedRef<ISequencer> InSequencer )
{
	return MakeShareable( new FTransformPropertyTrackEditor( InSequencer ) );
}


TSharedRef<ISequencerSection> FTransformPropertyTrackEditor::MakeSectionInterface(UMovieSceneSection& SectionObject, UMovieSceneTrack& Track, FGuid ObjectBinding)
{
	UMovieScenePropertyTrack* PropertyTrack = Cast<UMovieScenePropertyTrack>(&Track);
	checkf(PropertyTrack != nullptr, TEXT("Incompatible track in FTransformPropertyTrackEditor"));
	return MakeShareable(new FTransformPropertySection(GetSequencer().Get(), ObjectBinding, PropertyTrack->GetPropertyName(), PropertyTrack->GetPropertyPath(), SectionObject, Track.GetDisplayName()));
}

void FTransformPropertyTrackEditor::GenerateKeysFromPropertyChanged( const FPropertyChangedParams& PropertyChangedParams, TArray<FTransformKey>& NewGeneratedKeys, TArray<FTransformKey>& DefaultGeneratedKeys )
{
	FTransform Transform = PropertyChangedParams.GetPropertyValue<FTransform>();

	const FVector& Translation = Transform.GetTranslation();
	NewGeneratedKeys.Add(FTransformKey(EKey3DTransformChannel::Translation, EAxis::X, Translation.X, false));
	NewGeneratedKeys.Add(FTransformKey(EKey3DTransformChannel::Translation, EAxis::Y, Translation.Y, false));
	NewGeneratedKeys.Add(FTransformKey(EKey3DTransformChannel::Translation, EAxis::Z, Translation.Z, false));

	const FRotator& Rotator = Transform.GetRotation().Rotator();
	NewGeneratedKeys.Add(FTransformKey(EKey3DTransformChannel::Rotation, EAxis::X, Rotator.Roll, false));
	NewGeneratedKeys.Add(FTransformKey(EKey3DTransformChannel::Rotation, EAxis::Y, Rotator.Pitch, false));
	NewGeneratedKeys.Add(FTransformKey(EKey3DTransformChannel::Rotation, EAxis::Z, Rotator.Yaw, false));

	const FVector& Scale = Transform.GetScale3D();
	NewGeneratedKeys.Add(FTransformKey(EKey3DTransformChannel::Scale, EAxis::X, Scale.X, false));
	NewGeneratedKeys.Add(FTransformKey(EKey3DTransformChannel::Scale, EAxis::Y, Scale.Y, false));
	NewGeneratedKeys.Add(FTransformKey(EKey3DTransformChannel::Scale, EAxis::Z, Scale.Z, false));
}

