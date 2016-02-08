// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SequenceRecorderPrivatePCH.h"
#include "MovieScene3DTransformPropertyRecorder.h"
#include "MovieScene.h"
#include "MovieScene3DTransformSection.h"
#include "MovieScene3DTransformTrack.h"

static const FName TransformTrackName = TEXT("Transform");

void FMovieScene3DTransformPropertyRecorder::CreateSection(AActor* SourceActor, UMovieScene* MovieScene, const FGuid& Guid, bool bRecord) 
{
	ActorToRecord = SourceActor;

	UMovieScene3DTransformTrack* TransformTrack = MovieScene->AddTrack<UMovieScene3DTransformTrack>(Guid);
	if(TransformTrack)
	{
		TransformTrack->SetPropertyNameAndPath(TransformTrackName, TransformTrackName.ToString());

		MovieSceneSection = Cast<UMovieScene3DTransformSection>(TransformTrack->CreateNewSection());

		TransformTrack->AddSection(*MovieSceneSection);

		const bool bUnwindRotation = false;

		MovieSceneSection->SetDefault(FTransformKey(EKey3DTransformChannel::Translation, EAxis::X, 0.0f, bUnwindRotation));
		MovieSceneSection->SetDefault(FTransformKey(EKey3DTransformChannel::Translation, EAxis::Y, 0.0f, bUnwindRotation));
		MovieSceneSection->SetDefault(FTransformKey(EKey3DTransformChannel::Translation, EAxis::Z, 0.0f, bUnwindRotation));

		MovieSceneSection->SetDefault(FTransformKey(EKey3DTransformChannel::Rotation, EAxis::X, 0.0f, bUnwindRotation));
		MovieSceneSection->SetDefault(FTransformKey(EKey3DTransformChannel::Rotation, EAxis::Y, 0.0f, bUnwindRotation));
		MovieSceneSection->SetDefault(FTransformKey(EKey3DTransformChannel::Rotation, EAxis::Z, 0.0f, bUnwindRotation));

		MovieSceneSection->SetDefault(FTransformKey(EKey3DTransformChannel::Scale, EAxis::X, 1.0f, bUnwindRotation));
		MovieSceneSection->SetDefault(FTransformKey(EKey3DTransformChannel::Scale, EAxis::Y, 1.0f, bUnwindRotation));
		MovieSceneSection->SetDefault(FTransformKey(EKey3DTransformChannel::Scale, EAxis::Z, 1.0f, bUnwindRotation));

		MovieSceneSection->SetStartTime(0.0f);
	}

	bRecording = bRecord;
}

void FMovieScene3DTransformPropertyRecorder::FinalizeSection()
{
	bRecording = false;

	// now remove linear keys
	TPair<FRichCurve*, float> CurvesAndTolerances[] =
	{
		TPairInitializer<FRichCurve*, float>(&MovieSceneSection->GetTranslationCurve(EAxis::X), 0.1f),
		TPairInitializer<FRichCurve*, float>(&MovieSceneSection->GetTranslationCurve(EAxis::Y), 0.1f),
		TPairInitializer<FRichCurve*, float>(&MovieSceneSection->GetTranslationCurve(EAxis::Z), 0.1f),
		TPairInitializer<FRichCurve*, float>(&MovieSceneSection->GetRotationCurve(EAxis::X), 0.25f),
		TPairInitializer<FRichCurve*, float>(&MovieSceneSection->GetRotationCurve(EAxis::Y), 0.25f),
		TPairInitializer<FRichCurve*, float>(&MovieSceneSection->GetRotationCurve(EAxis::Z), 0.25f),
		TPairInitializer<FRichCurve*, float>(&MovieSceneSection->GetScaleCurve(EAxis::X), 0.1f),
		TPairInitializer<FRichCurve*, float>(&MovieSceneSection->GetScaleCurve(EAxis::Y), 0.1f),
		TPairInitializer<FRichCurve*, float>(&MovieSceneSection->GetScaleCurve(EAxis::Z), 0.1f),
	};

	for(TPair<FRichCurve*, float>& CurveAndTolerance : CurvesAndTolerances)
	{
		CurveAndTolerance.Key->RemoveRedundantKeys(CurveAndTolerance.Value);
	}
}

void FMovieScene3DTransformPropertyRecorder::Record(float CurrentTime)
{
	if(ActorToRecord.IsValid())
	{
		MovieSceneSection->SetEndTime(CurrentTime);

		if(bRecording)
		{
			const bool bUnwindRotation = false;

			USkeletalMeshComponent* SkeletalMeshComponent = ActorToRecord->FindComponentByClass<USkeletalMeshComponent>();

			const FTransform Transform = SkeletalMeshComponent ? SkeletalMeshComponent->ComponentToWorld : ActorToRecord->GetTransform();
			const FVector Translation = Transform.GetTranslation();
			const FVector Rotation = Transform.GetRotation().Rotator().Euler();
			const FVector Scale = Transform.GetScale3D();

			MovieSceneSection->AddKey(CurrentTime, FTransformKey(EKey3DTransformChannel::Translation, EAxis::X, Translation.X, bUnwindRotation), EMovieSceneKeyInterpolation::Auto);
			MovieSceneSection->AddKey(CurrentTime, FTransformKey(EKey3DTransformChannel::Translation, EAxis::Y, Translation.Y, bUnwindRotation), EMovieSceneKeyInterpolation::Auto);
			MovieSceneSection->AddKey(CurrentTime, FTransformKey(EKey3DTransformChannel::Translation, EAxis::Z, Translation.Z, bUnwindRotation), EMovieSceneKeyInterpolation::Auto);

			MovieSceneSection->AddKey(CurrentTime, FTransformKey(EKey3DTransformChannel::Rotation, EAxis::X, Rotation.X, bUnwindRotation), EMovieSceneKeyInterpolation::Auto);
			MovieSceneSection->AddKey(CurrentTime, FTransformKey(EKey3DTransformChannel::Rotation, EAxis::Y, Rotation.Y, bUnwindRotation), EMovieSceneKeyInterpolation::Auto);
			MovieSceneSection->AddKey(CurrentTime, FTransformKey(EKey3DTransformChannel::Rotation, EAxis::Z, Rotation.Z, bUnwindRotation), EMovieSceneKeyInterpolation::Auto);

			MovieSceneSection->AddKey(CurrentTime, FTransformKey(EKey3DTransformChannel::Scale, EAxis::X, Scale.X, bUnwindRotation), EMovieSceneKeyInterpolation::Auto);
			MovieSceneSection->AddKey(CurrentTime, FTransformKey(EKey3DTransformChannel::Scale, EAxis::Y, Scale.Y, bUnwindRotation), EMovieSceneKeyInterpolation::Auto);
			MovieSceneSection->AddKey(CurrentTime, FTransformKey(EKey3DTransformChannel::Scale, EAxis::Z, Scale.Z, bUnwindRotation), EMovieSceneKeyInterpolation::Auto);
		}
	}
}