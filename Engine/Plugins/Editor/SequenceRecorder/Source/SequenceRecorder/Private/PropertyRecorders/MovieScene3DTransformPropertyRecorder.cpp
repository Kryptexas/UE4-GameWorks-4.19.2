// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SequenceRecorderPrivatePCH.h"
#include "MovieScene3DTransformPropertyRecorder.h"
#include "MovieScene.h"
#include "MovieScene3DTransformSection.h"
#include "MovieScene3DTransformTrack.h"
#include "MovieSceneAnimationPropertyRecorder.h"
#include "SequenceRecorderUtils.h"

void FMovieScene3DTransformPropertyRecorder::CreateSection(UObject* InObjectToRecord, UMovieScene* MovieScene, const FGuid& Guid, float Time, bool bRecord) 
{
	ObjectToRecord = InObjectToRecord;

	UMovieScene3DTransformTrack* TransformTrack = MovieScene->AddTrack<UMovieScene3DTransformTrack>(Guid);
	if(TransformTrack)
	{
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

		MovieSceneSection->SetStartTime(Time);
	}

	bRecording = bRecord;

	Record(Time);
}

static void WindRelativeRotators(float& Axis0, float& Axis1)
{
	float Diff = Axis0 - Axis1;
	float AbsDiff = FMath::Abs(Diff);
	if(AbsDiff > 180.0f)
	{
		Axis1 += 360.0f * FMath::Sign(Diff) * FMath::FloorToFloat((AbsDiff / 360.0f) + 0.5f);
	}
}

void FMovieScene3DTransformPropertyRecorder::FinalizeSection()
{
	bRecording = false;

	// if we have a valid animation recorder, we need to build our transforms from the animation
	// so we properly synchronize our keyframes
	if(AnimRecorder.IsValid())
	{
		check(BufferedTransforms.Num() == 0);

		UAnimSequence* AnimSequence = AnimRecorder->GetSequence();
		USkeletalMesh* SkeletalMesh = AnimRecorder->GetSkeletalMesh();
		if(AnimSequence && SkeletalMesh)
		{
			// find the root bone
			int32 RootIndex = INDEX_NONE;
			USkeleton* AnimSkeleton = AnimSequence->GetSkeleton();
			for (int32 TrackIndex = 0; TrackIndex < AnimSequence->RawAnimationData.Num(); ++TrackIndex)
			{
				// verify if this bone exists in skeleton
				int32 BoneTreeIndex = AnimSequence->GetSkeletonIndexFromTrackIndex(TrackIndex);
				if (BoneTreeIndex != INDEX_NONE)
				{
					int32 BoneIndex = AnimSkeleton->GetMeshBoneIndexFromSkeletonBoneIndex(SkeletalMesh, BoneTreeIndex);
					int32 ParentIndex = SkeletalMesh->RefSkeleton.GetParentIndex(BoneIndex);
					if(ParentIndex == INDEX_NONE)
					{
						// found root
						RootIndex = BoneIndex;
						break;
					}
				}
			}

			check(RootIndex != INDEX_NONE);

			const float StartTime = MovieSceneSection->GetStartTime();

			// we may need to offset the transform here if the animation was not recorded on the root component
			FTransform InvComponentTransform = AnimRecorder->GetComponentTransform().Inverse();

			FRawAnimSequenceTrack& RawTrack = AnimSequence->RawAnimationData[RootIndex];
			const int32 KeyCount = FMath::Max(FMath::Max(RawTrack.PosKeys.Num(), RawTrack.RotKeys.Num()), RawTrack.ScaleKeys.Num());
			for(int32 KeyIndex = 0; KeyIndex < KeyCount; KeyIndex++)
			{
				FTransform Transform;
				if(RawTrack.PosKeys.IsValidIndex(KeyIndex))
				{
					Transform.SetTranslation(RawTrack.PosKeys[KeyIndex]);
				}
				else if(RawTrack.PosKeys.Num() > 0)
				{
					Transform.SetTranslation(RawTrack.PosKeys[0]);
				}
				
				if(RawTrack.RotKeys.IsValidIndex(KeyIndex))
				{
					Transform.SetRotation(RawTrack.RotKeys[KeyIndex]);
				}
				else if(RawTrack.RotKeys.Num() > 0)
				{
					Transform.SetRotation(RawTrack.RotKeys[0]);
				}

				if(RawTrack.ScaleKeys.IsValidIndex(KeyIndex))
				{
					Transform.SetScale3D(RawTrack.ScaleKeys[KeyIndex]);
				}
				else if(RawTrack.ScaleKeys.Num() > 0)
				{
					Transform.SetScale3D(RawTrack.ScaleKeys[0]);
				}

				BufferedTransforms.Add(FBufferedTransformKey(InvComponentTransform * Transform, StartTime + AnimSequence->GetTimeAtFrame(KeyIndex)));
			}
		}
	}

	// Try to 're-wind' rotations that look like axis flips
	// We need to do this as a post-process because the recorder cant reliably access 'wound' rotations:
	// - Net quantize may use quaternions.
	// - Scene components cache transforms as quaternions.
	// - Gameplay is free to clamp/fmod rotations as it sees fit.
	int32 TransformCount = BufferedTransforms.Num();
	for(int32 TransformIndex = 0; TransformIndex < TransformCount - 1; TransformIndex++)
	{
		FRotator& Rotator = BufferedTransforms[TransformIndex].WoundRotation;
		FRotator& NextRotator = BufferedTransforms[TransformIndex + 1].WoundRotation;

		WindRelativeRotators(Rotator.Pitch, NextRotator.Pitch);
		WindRelativeRotators(Rotator.Yaw, NextRotator.Yaw);
		WindRelativeRotators(Rotator.Roll, NextRotator.Roll);
	}

	// add buffered transforms
	const bool bUnwindRotation = false;
	for(const FBufferedTransformKey& BufferedTransform : BufferedTransforms)
	{
		const FVector Translation = BufferedTransform.Transform.GetTranslation();
		const FVector Rotation = BufferedTransform.WoundRotation.Euler();
		const FVector Scale = BufferedTransform.Transform.GetScale3D();

		MovieSceneSection->AddKey(BufferedTransform.KeyTime, FTransformKey(EKey3DTransformChannel::Translation, EAxis::X, Translation.X, bUnwindRotation), EMovieSceneKeyInterpolation::Linear);
		MovieSceneSection->AddKey(BufferedTransform.KeyTime, FTransformKey(EKey3DTransformChannel::Translation, EAxis::Y, Translation.Y, bUnwindRotation), EMovieSceneKeyInterpolation::Linear);
		MovieSceneSection->AddKey(BufferedTransform.KeyTime, FTransformKey(EKey3DTransformChannel::Translation, EAxis::Z, Translation.Z, bUnwindRotation), EMovieSceneKeyInterpolation::Linear);

		MovieSceneSection->AddKey(BufferedTransform.KeyTime, FTransformKey(EKey3DTransformChannel::Rotation, EAxis::X, Rotation.X, bUnwindRotation), EMovieSceneKeyInterpolation::Linear);
		MovieSceneSection->AddKey(BufferedTransform.KeyTime, FTransformKey(EKey3DTransformChannel::Rotation, EAxis::Y, Rotation.Y, bUnwindRotation), EMovieSceneKeyInterpolation::Linear);
		MovieSceneSection->AddKey(BufferedTransform.KeyTime, FTransformKey(EKey3DTransformChannel::Rotation, EAxis::Z, Rotation.Z, bUnwindRotation), EMovieSceneKeyInterpolation::Linear);

		MovieSceneSection->AddKey(BufferedTransform.KeyTime, FTransformKey(EKey3DTransformChannel::Scale, EAxis::X, Scale.X, bUnwindRotation), EMovieSceneKeyInterpolation::Linear);
		MovieSceneSection->AddKey(BufferedTransform.KeyTime, FTransformKey(EKey3DTransformChannel::Scale, EAxis::Y, Scale.Y, bUnwindRotation), EMovieSceneKeyInterpolation::Linear);
		MovieSceneSection->AddKey(BufferedTransform.KeyTime, FTransformKey(EKey3DTransformChannel::Scale, EAxis::Z, Scale.Z, bUnwindRotation), EMovieSceneKeyInterpolation::Linear);
	}

	BufferedTransforms.Empty();

	// now remove linear keys
	TPair<FRichCurve*, float> CurvesAndTolerances[] =
	{
		TPairInitializer<FRichCurve*, float>(&MovieSceneSection->GetTranslationCurve(EAxis::X), KINDA_SMALL_NUMBER),
		TPairInitializer<FRichCurve*, float>(&MovieSceneSection->GetTranslationCurve(EAxis::Y), KINDA_SMALL_NUMBER),
		TPairInitializer<FRichCurve*, float>(&MovieSceneSection->GetTranslationCurve(EAxis::Z), KINDA_SMALL_NUMBER),
		TPairInitializer<FRichCurve*, float>(&MovieSceneSection->GetRotationCurve(EAxis::X), KINDA_SMALL_NUMBER),
		TPairInitializer<FRichCurve*, float>(&MovieSceneSection->GetRotationCurve(EAxis::Y), KINDA_SMALL_NUMBER),
		TPairInitializer<FRichCurve*, float>(&MovieSceneSection->GetRotationCurve(EAxis::Z), KINDA_SMALL_NUMBER),
		TPairInitializer<FRichCurve*, float>(&MovieSceneSection->GetScaleCurve(EAxis::X), KINDA_SMALL_NUMBER),
		TPairInitializer<FRichCurve*, float>(&MovieSceneSection->GetScaleCurve(EAxis::Y), KINDA_SMALL_NUMBER),
		TPairInitializer<FRichCurve*, float>(&MovieSceneSection->GetScaleCurve(EAxis::Z), KINDA_SMALL_NUMBER),
	};

	for(TPair<FRichCurve*, float>& CurveAndTolerance : CurvesAndTolerances)
	{
		CurveAndTolerance.Key->RemoveRedundantKeys(CurveAndTolerance.Value);
	}
}

void FMovieScene3DTransformPropertyRecorder::Record(float CurrentTime)
{
	if(ObjectToRecord.IsValid())
	{
		if(USceneComponent* SceneComponent = Cast<USceneComponent>(ObjectToRecord.Get()))
		{
			// dont record non-registered scene components
			if(!SceneComponent->IsRegistered())
			{
				return;
			}
		}

		MovieSceneSection->SetEndTime(CurrentTime);

		if(bRecording)
		{
			// don't record from the transform of the component/actor if we are synchronizing with an animation
			if(!AnimRecorder.IsValid())
			{
				FTransform Transform;
				if(USceneComponent* SceneComponent = Cast<USceneComponent>(ObjectToRecord.Get()))
				{
					Transform = SceneComponent->GetRelativeTransform();
				}
				else if(AActor* Actor = Cast<AActor>(ObjectToRecord.Get()))
				{
					FName SocketName, ComponentName;
					if(SequenceRecorderUtils::GetAttachment(Actor, SocketName, ComponentName) != nullptr && Actor->GetRootComponent() != nullptr)
					{
						Transform = Actor->GetRootComponent()->GetRelativeTransform();
					}
					else
					{
						Transform = Actor->GetTransform();
					}
				}
				BufferedTransforms.Add(FBufferedTransformKey(Transform, CurrentTime));
			}
		}
	}
}