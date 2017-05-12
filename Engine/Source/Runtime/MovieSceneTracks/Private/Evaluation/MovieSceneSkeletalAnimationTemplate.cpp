// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Evaluation/MovieSceneSkeletalAnimationTemplate.h"

#include "Compilation/MovieSceneCompilerRules.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Runtime/AnimGraphRuntime/Public/AnimSequencerInstance.h"
#include "MovieSceneEvaluation.h"
#include "IMovieScenePlayer.h"


bool ShouldUsePreviewPlayback(IMovieScenePlayer& Player, UObject& RuntimeObject)
{
	// we also use PreviewSetAnimPosition in PIE when not playing, as we can preview in PIE
	bool bIsNotInPIEOrNotPlaying = (RuntimeObject.GetWorld() && !RuntimeObject.GetWorld()->HasBegunPlay()) || Player.GetPlaybackStatus() != EMovieScenePlayerStatus::Playing;
	return GIsEditor && bIsNotInPIEOrNotPlaying;
}

bool CanPlayAnimation(USkeletalMeshComponent* SkeletalMeshComponent, UAnimSequenceBase* AnimAssetBase = nullptr)
{
	return (SkeletalMeshComponent->SkeletalMesh && SkeletalMeshComponent->SkeletalMesh->Skeleton && 
		(!AnimAssetBase || SkeletalMeshComponent->SkeletalMesh->Skeleton->IsCompatible(AnimAssetBase->GetSkeleton())));
}

struct FMinimalAnimParameters
{
	FMinimalAnimParameters(UAnimSequenceBase* InAnimation, float InEvalTime, float InBlendWeight, uint32 InSectionId, FName InSlotName)
		: Animation(InAnimation)
		, EvalTime(InEvalTime)
		, BlendWeight(InBlendWeight)
		, SectionId(InSectionId)
		, SlotName(InSlotName)
	{}
	
	UAnimSequenceBase* Animation;
	float EvalTime;
	float BlendWeight;
	uint32 SectionId;
	FName SlotName;
};

struct FPreAnimatedAnimationTokenProducer : IMovieScenePreAnimatedTokenProducer
{
	virtual IMovieScenePreAnimatedTokenPtr CacheExistingState(UObject& Object) const
	{
		struct FToken : IMovieScenePreAnimatedToken
		{
			FToken(USkeletalMeshComponent* InComponent)
			{
				// Cache this object's current update flag and animation mode
				MeshComponentUpdateFlag = InComponent->MeshComponentUpdateFlag;
				AnimationMode = InComponent->GetAnimationMode();
			}

			virtual void RestoreState(UObject& ObjectToRestore, IMovieScenePlayer& Player)
			{
				USkeletalMeshComponent* Component = CastChecked<USkeletalMeshComponent>(&ObjectToRestore);

				UAnimSequencerInstance* SequencerInst = Cast<UAnimSequencerInstance>(Component->GetAnimInstance());
				if (SequencerInst)
				{
					SequencerInst->ResetNodes();
				}

				UAnimSequencerInstance::UnbindFromSkeletalMeshComponent(Component);

				// Reset the mesh component update flag and animation mode to what they were before we animated the object
				Component->MeshComponentUpdateFlag = MeshComponentUpdateFlag;
				Component->SetAnimationMode(AnimationMode);

			}

			EMeshComponentUpdateFlag::Type MeshComponentUpdateFlag;
			EAnimationMode::Type AnimationMode;
		};

		return FToken(CastChecked<USkeletalMeshComponent>(&Object));
	}
};

struct FSkeletalAnimationTrackData : IPersistentEvaluationData
{
	struct FComponentDataContainer
	{
		FComponentDataContainer(USkeletalMeshComponent* InComponent) : Component(InComponent) {}

		// per slot name, montage reference if the component is using this
		TMap<FName, TWeakObjectPtr<class UAnimMontage>> CurrentlyPlayingMontages;

		// component reference
		TWeakObjectPtr<USkeletalMeshComponent> Component;

		// parameters to apply per section
		TArray<FMinimalAnimParameters, TInlineAllocator<1>> SectionParams;
	};

	TMap<FObjectKey, FComponentDataContainer> ComponentDataContainer;

	// Store the section parameters and map them to the skeletal component
	void AssignSkeletalAnimationData(USkeletalMeshComponent* SkeletalMeshComponent, FMinimalAnimParameters AnimParameters)
	{
		check(SkeletalMeshComponent);

		FObjectKey ObjectKey(SkeletalMeshComponent);

		FComponentDataContainer* ComponentData = ComponentDataContainer.Find(ObjectKey);
		if (!ComponentData)
		{
			// add to cache data container
			ComponentData = &ComponentDataContainer.Add(ObjectKey, FComponentDataContainer(SkeletalMeshComponent));
		}

		ComponentData->SectionParams.Add(AnimParameters);
	}

	// Apply the skeletal animation 
	void ApplySkeletalAnimationData(const FMovieSceneContext& Context, const FMovieSceneEvaluationOperand& Operand, IMovieScenePlayer& Player)
	{
		for (auto& Pair : ComponentDataContainer)
		{
			const FComponentDataContainer& DataContainer = Pair.Value;
			USkeletalMeshComponent* SkeletalMeshComponent = DataContainer.Component.Get();
			
			if (SkeletalMeshComponent)
			{
				UAnimCustomInstance::BindToSkeletalMeshComponent<UAnimSequencerInstance>(SkeletalMeshComponent);

				static const bool bLooping = false;

				bool bPreviewPlayback = ShouldUsePreviewPlayback(Player, *SkeletalMeshComponent);

				const EMovieScenePlayerStatus::Type PlayerStatus = Player.GetPlaybackStatus();

				// If the playback status is jumping, ie. one such occurrence is setting the time for thumbnail generation, disable anim notifies updates because it could fire audio
				const bool bFireNotifies = !bPreviewPlayback || (PlayerStatus != EMovieScenePlayerStatus::Jumping && PlayerStatus != EMovieScenePlayerStatus::Stopped);

				// When jumping from one cut to another cut, the delta time should be 0 so that anim notifies before the current position are not evaluated. Note, anim notifies at the current time should still be evaluated.
				const float DeltaTime = Context.HasJumped() ? 0.f : Context.GetRange().Size<float>();

				const bool bResetDynamics = PlayerStatus == EMovieScenePlayerStatus::Stepping || 
											PlayerStatus == EMovieScenePlayerStatus::Jumping || 
											PlayerStatus == EMovieScenePlayerStatus::Scrubbing || 
											(DeltaTime == 0.0f && PlayerStatus != EMovieScenePlayerStatus::Stopped); 
			
				for (const FMinimalAnimParameters& SectionParam : DataContainer.SectionParams)
				{
					UAnimSequenceBase* AnimSequence = SectionParam.Animation;
					float EvalTime = SectionParam.EvalTime;
					float Weight = SectionParam.BlendWeight;

					uint32 SectionId = SectionParam.SectionId;

					if (bPreviewPlayback)
					{
						PreviewSetAnimPosition(SkeletalMeshComponent, SectionParam.SlotName, SectionId, AnimSequence, EvalTime, Weight, bLooping, bFireNotifies, DeltaTime, Player.GetPlaybackStatus() == EMovieScenePlayerStatus::Playing, bResetDynamics);
					}
					else
					{
						SetAnimPosition(SkeletalMeshComponent, SectionParam.SlotName, SectionId, AnimSequence, EvalTime, Weight, bLooping, bFireNotifies);
					}
				}
			}
		}

		// Clear all existing section data ready for the next frame
		for (auto& Pair : ComponentDataContainer)
		{
			Pair.Value.SectionParams.Empty();
		}
	}

	void StopAnimationForSlot(USkeletalMeshComponent* SkeletalMeshComponent, FName SlotName)
	{
		UAnimInstance* AnimInstance = SkeletalMeshComponent->GetAnimInstance();
		UAnimMontage* Montage = GetCurrentlyPlayingMontage(SkeletalMeshComponent, SlotName).Get();
		if (AnimInstance && Montage)
		{
			AnimInstance->Montage_Stop(0.f, Montage);
		}
	}

private:

	TWeakObjectPtr<class UAnimMontage>& GetCurrentlyPlayingMontage(USkeletalMeshComponent* SkeletalMeshComponent, const FName& SlotName)
	{
		FComponentDataContainer* DataContainer = ComponentDataContainer.Find(SkeletalMeshComponent);
		check(DataContainer);
		return DataContainer->CurrentlyPlayingMontages.FindOrAdd(SlotName);
	}

	void SetAnimPosition(USkeletalMeshComponent* SkeletalMeshComponent, FName SlotName, int32 SequenceIndex, UAnimSequenceBase* InAnimSequence, float InPosition, float Weight, bool bLooping, bool bFireNotifies)
	{
		if (CanPlayAnimation(SkeletalMeshComponent, InAnimSequence))
		{
			UAnimSequencerInstance* SequencerInst = Cast<UAnimSequencerInstance>(SkeletalMeshComponent->GetAnimInstance());

			if (SequencerInst)
			{
				// Set position and weight
				SequencerInst->UpdateAnimTrack(InAnimSequence, SequenceIndex, InPosition, Weight, bFireNotifies);
			}
			else
			{
				TWeakObjectPtr<class UAnimMontage>& CurrentlyPlayingMontage = GetCurrentlyPlayingMontage(SkeletalMeshComponent, SlotName);
				CurrentlyPlayingMontage = FAnimMontageInstance::SetMatineeAnimPositionInner(SlotName, SkeletalMeshComponent, InAnimSequence, InPosition, bLooping);

				// Ensure the sequence is not stopped
				UAnimInstance* AnimInst = SkeletalMeshComponent->GetAnimInstance();
				if (AnimInst && CurrentlyPlayingMontage.IsValid())
				{
					AnimInst->Montage_Resume(CurrentlyPlayingMontage.Get());
				}
			}
		}
	}

	void PreviewSetAnimPosition(USkeletalMeshComponent* SkeletalMeshComponent, FName SlotName, int32 SequenceIndex, UAnimSequenceBase* InAnimSequence, float InPosition, float Weight, bool bLooping, bool bFireNotifies, float DeltaTime, bool bPlaying, bool bResetDynamics)
	{
		if(!CanPlayAnimation(SkeletalMeshComponent, InAnimSequence))
		{
			return;
		}

		UAnimSequencerInstance* SequencerInst = Cast<UAnimSequencerInstance>(SkeletalMeshComponent->GetAnimInstance());

		if (SequencerInst)
		{
			// Set position and weight
			SequencerInst->UpdateAnimTrack(InAnimSequence, SequenceIndex, InPosition, Weight, bFireNotifies);

			// Update space bases so new animation position has an effect.
			// @todo - hack - this will be removed at some point
// 			SkeletalMeshComponent->TickAnimation(0.03f, false);
// 
// 			SkeletalMeshComponent->RefreshBoneTransforms();
// 			SkeletalMeshComponent->RefreshSlaveComponents();
// 			SkeletalMeshComponent->UpdateComponentToWorld();
// 			SkeletalMeshComponent->FinalizeBoneTransform();
// 			SkeletalMeshComponent->MarkRenderTransformDirty();
// 			SkeletalMeshComponent->MarkRenderDynamicDataDirty();
		}
		else
		{
			TWeakObjectPtr<class UAnimMontage>& CurrentlyPlayingMontage = GetCurrentlyPlayingMontage(SkeletalMeshComponent, SlotName);
			CurrentlyPlayingMontage = FAnimMontageInstance::PreviewMatineeSetAnimPositionInner(SlotName, SkeletalMeshComponent, InAnimSequence, InPosition, bLooping, bFireNotifies, DeltaTime);

			// add to montage
			// if we are not playing, make sure we dont continue (as skeletal meshes can still tick us onwards)
			UAnimInstance* AnimInst = SkeletalMeshComponent->GetAnimInstance();
			if (AnimInst)
			{
				if (CurrentlyPlayingMontage.IsValid())
				{
					if (bPlaying)
					{
						AnimInst->Montage_Resume(CurrentlyPlayingMontage.Get());
					}
					else
					{
						AnimInst->Montage_Pause(CurrentlyPlayingMontage.Get());
					}
				}

				if (bResetDynamics)
				{
					// make sure we reset any simulations
					AnimInst->ResetDynamics();
				}
			}
		}
	}
};

struct FAnimationSectionData : IPersistentEvaluationData
{
	TSet<FObjectKey> AllAnimatedComponents;
};


// Execution token that assigns blended animation to all skeletal mesh components for this section
struct FSkeletalAnimationAccumulatorToken : IMovieSceneExecutionToken
{
	FSkeletalAnimationAccumulatorToken(const FMinimalAnimParameters& InAnimParams) : AnimParams(InAnimParams){}

	virtual void Execute(const FMovieSceneContext& Context, const FMovieSceneEvaluationOperand& Operand, FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) override
	{
		FSkeletalAnimationTrackData& SharedData = PersistentData.GetOrAdd<FSkeletalAnimationTrackData>(FMovieSceneSkeletalAnimationSharedTrack::GetSharedDataKey());

		FAnimationSectionData& ThisSectionData = PersistentData.GetOrAddSectionData<FAnimationSectionData>();

		for (TWeakObjectPtr<> Object : Player.FindBoundObjects(Operand))
		{
			UObject* ObjectPtr = Object.Get();
			if (!ObjectPtr)
			{
				continue;
			}

			// first try to control the component directly
			USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(ObjectPtr);
			if (!SkeletalMeshComponent)
			{
				// then check to see if we are controlling an actor & if so use its first USkeletalMeshComponent 
				if (AActor* Actor = Cast<AActor>(ObjectPtr))
				{
					SkeletalMeshComponent = Actor->FindComponentByClass<USkeletalMeshComponent>();
				}
			}

			if (SkeletalMeshComponent)
			{
				// Save preanimated state here rather than the shared track as the shared track will still be evaluated even if there are no animations happening
				Player.SavePreAnimatedState(*SkeletalMeshComponent, TMovieSceneAnimTypeID<FSkeletalAnimationAccumulatorToken>(), FPreAnimatedAnimationTokenProducer());
				ThisSectionData.AllAnimatedComponents.Add(SkeletalMeshComponent);

				SharedData.AssignSkeletalAnimationData(SkeletalMeshComponent, AnimParams);
			}
		}
	}

	FMinimalAnimParameters AnimParams;
};

FMovieSceneSkeletalAnimationSectionTemplate::FMovieSceneSkeletalAnimationSectionTemplate(const UMovieSceneSkeletalAnimationSection& InSection)
	: Params(InSection.Params, InSection.GetStartTime(), InSection.GetEndTime())
{
}

void FMovieSceneSkeletalAnimationSectionTemplate::Evaluate(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const
{
	if (Params.Animation)
	{
		// calculate the time at which to evaluate the animation
		float EvalTime = Params.MapTimeToAnimation(Context.GetTime());
		float BlendWeight = Params.Weight.Eval(Context.GetTime());

		// Create the execution token with the minimal anim params
		FMinimalAnimParameters AnimParams(
			Params.Animation, EvalTime, BlendWeight, GetTypeHash(PersistentData.GetSectionKey()), Params.SlotName
			);
		ExecutionTokens.Add(FSkeletalAnimationAccumulatorToken(AnimParams));
	}
}

void FMovieSceneSkeletalAnimationSectionTemplate::TearDown(FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) const
{
	// Reset blend weights on any nodes that we animated
	FAnimationSectionData* ThisSectionData = PersistentData.FindSectionData<FAnimationSectionData>();
	FSkeletalAnimationTrackData* SharedData = PersistentData.Find<FSkeletalAnimationTrackData>(FMovieSceneSkeletalAnimationSharedTrack::GetSharedDataKey());

	if (ThisSectionData)
	{
		for (FObjectKey Key : ThisSectionData->AllAnimatedComponents)
		{
			if (USkeletalMeshComponent* Component = Cast<USkeletalMeshComponent>(Key.ResolveObjectPtr()))
			{
				UAnimSequencerInstance* SequencerInst = Cast<UAnimSequencerInstance>(Component->GetAnimInstance());
				if (SequencerInst)
				{
					SequencerInst->ResetNodes();
				}

				if (SharedData)
				{
					SharedData->StopAnimationForSlot(Component, Params.SlotName);
				}
			}
		}
	}
}

float FMovieSceneSkeletalAnimationSectionTemplateParameters::MapTimeToAnimation(float ThisPosition) const
{
	// @todo: Sequencer: what is this for??
	//ThisPosition -= 1 / 1000.0f;

	ThisPosition = FMath::Clamp(ThisPosition, SectionStartTime, SectionEndTime);

	const float SectionPlayRate = PlayRate;
	const float AnimPlayRate = FMath::IsNearlyZero(SectionPlayRate) ? 1.0f : SectionPlayRate;

	const float SeqLength = GetSequenceLength() - (StartOffset + EndOffset);

	ThisPosition = (ThisPosition - SectionStartTime) * AnimPlayRate;
	if (SeqLength > 0.f)
	{
		ThisPosition = FMath::Fmod(ThisPosition, SeqLength);
	}
	ThisPosition += StartOffset;
	if (bReverse)
	{
		ThisPosition = (SeqLength - (ThisPosition - StartOffset)) + StartOffset;
	}

	return ThisPosition;
}

struct FSkeletalAnimationExecutionToken : IMovieSceneExecutionToken
{
	virtual void Execute(const FMovieSceneContext& Context, const FMovieSceneEvaluationOperand& Operand, FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) override
	{
		FSkeletalAnimationTrackData* TrackData = PersistentData.Find<FSkeletalAnimationTrackData>(FMovieSceneSkeletalAnimationSharedTrack::GetSharedDataKey());
		if (TrackData)
		{
			TrackData->ApplySkeletalAnimationData(Context, Operand, Player);
		}
	}
};

void FMovieSceneSkeletalAnimationSharedTrack::Evaluate(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const
{
	ExecutionTokens.Add(FSkeletalAnimationExecutionToken());
}

FSharedPersistentDataKey FMovieSceneSkeletalAnimationSharedTrack::GetSharedDataKey()
{
	static FMovieSceneSharedDataId DataId(FMovieSceneSharedDataId::Allocate());
	return FSharedPersistentDataKey(DataId, FMovieSceneEvaluationOperand());
}