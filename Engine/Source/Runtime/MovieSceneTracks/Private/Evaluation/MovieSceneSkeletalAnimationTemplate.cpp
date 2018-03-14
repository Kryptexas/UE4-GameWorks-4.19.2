// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "Evaluation/MovieSceneSkeletalAnimationTemplate.h"

#include "Compilation/MovieSceneCompilerRules.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Runtime/AnimGraphRuntime/Public/AnimSequencerInstance.h"
#include "MovieSceneEvaluation.h"
#include "IMovieScenePlayer.h"
#include "ObjectKey.h"

bool ShouldUsePreviewPlayback(IMovieScenePlayer& Player, UObject& RuntimeObject)
{
	// we also use PreviewSetAnimPosition in PIE when not playing, as we can preview in PIE
	bool bIsNotInPIEOrNotPlaying = (RuntimeObject.GetWorld() && !RuntimeObject.GetWorld()->HasBegunPlay()) || Player.GetPlaybackStatus() != EMovieScenePlayerStatus::Playing;
	return GIsEditor && bIsNotInPIEOrNotPlaying;
}

bool CanPlayAnimation(USkeletalMeshComponent* SkeletalMeshComponent, UAnimSequenceBase* AnimAssetBase)
{
	return (SkeletalMeshComponent->SkeletalMesh && SkeletalMeshComponent->SkeletalMesh->Skeleton && 
		(!AnimAssetBase || SkeletalMeshComponent->SkeletalMesh->Skeleton->IsCompatible(AnimAssetBase->GetSkeleton())));
}

void ResetAnimSequencerInstance(UObject& ObjectToRestore, IMovieScenePlayer& Player)
{
	CastChecked<UAnimSequencerInstance>(&ObjectToRestore)->ResetNodes();
}

struct FStopPlayingMontageTokenProducer : IMovieScenePreAnimatedTokenProducer
{
	TWeakObjectPtr<UAnimInstance> TempInstance;
	int32 TempMontageInstanceId;

	FStopPlayingMontageTokenProducer(TWeakObjectPtr<UAnimInstance> InTempInstance, int32 InTempMontageInstanceId)
	: TempInstance(InTempInstance)
	, TempMontageInstanceId(InTempMontageInstanceId){}

	virtual IMovieScenePreAnimatedTokenPtr CacheExistingState(UObject& Object) const
	{
		struct FToken : IMovieScenePreAnimatedToken
		{
			TWeakObjectPtr<UAnimInstance> WeakInstance;
			int32 MontageInstanceId;

			FToken(TWeakObjectPtr<UAnimInstance> InWeakInstance, int32 InMontageInstanceId) 
			: WeakInstance(InWeakInstance)
			, MontageInstanceId(InMontageInstanceId) {}

			virtual void RestoreState(UObject& ObjectToRestore, IMovieScenePlayer& Player) override
			{
				UAnimInstance* AnimInstance = WeakInstance.Get();
				if (AnimInstance)
				{
					FAnimMontageInstance* MontageInstance = AnimInstance->GetMontageInstanceForID(MontageInstanceId);
					if (MontageInstance)
					{
						MontageInstance->Stop(FAlphaBlend(0.f), false);
					}
				}
			}
		};

		return FToken(TempInstance, TempMontageInstanceId);
	}
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
				if (Component->GetAnimationMode() != AnimationMode)
				{
					// this SetAnimationMode reinitializes even if the mode is same
					// if we're using same anim blueprint, we don't want to keep reinitializing it. 
					Component->SetAnimationMode(AnimationMode);
				}
			}

			EMeshComponentUpdateFlag::Type MeshComponentUpdateFlag;
			EAnimationMode::Type AnimationMode;
		};

		return FToken(CastChecked<USkeletalMeshComponent>(&Object));
	}
};


struct FMinimalAnimParameters
{
	FMinimalAnimParameters(UAnimSequenceBase* InAnimation, float InEvalTime, float InBlendWeight, const FMovieSceneEvaluationScope& InScope, FName InSlotName, FObjectKey InSection)
		: Animation(InAnimation)
		, EvalTime(InEvalTime)
		, BlendWeight(InBlendWeight)
		, EvaluationScope(InScope)
		, SlotName(InSlotName)
		, Section(InSection)
	{}
	
	UAnimSequenceBase* Animation;
	float EvalTime;
	float BlendWeight;
	FMovieSceneEvaluationScope EvaluationScope;
	FName SlotName;
	FObjectKey Section;
};

/** Montage player per section data */
struct FMontagePlayerPerSectionData 
{
	TWeakObjectPtr<UAnimMontage> Montage;
	int32 MontageInstanceId;
};

namespace MovieScene
{
	struct FBlendedAnimation
	{
		TArray<FMinimalAnimParameters> AllAnimations;

		FBlendedAnimation& Resolve(TMovieSceneInitialValueStore<FBlendedAnimation>& InitialValueStore)
		{
			return *this;
		}
	};

	void BlendValue(FBlendedAnimation& OutBlend, const FMinimalAnimParameters& InValue, float Weight, EMovieSceneBlendType BlendType, TMovieSceneInitialValueStore<FBlendedAnimation>& InitialValueStore)
	{
		OutBlend.AllAnimations.Add(InValue);
	}

	struct FComponentAnimationActuator : TMovieSceneBlendingActuator<FBlendedAnimation>
	{
		FComponentAnimationActuator() : TMovieSceneBlendingActuator<FBlendedAnimation>(GetActuatorTypeID()) {}

		static FMovieSceneBlendingActuatorID GetActuatorTypeID()
		{
			static FMovieSceneAnimTypeID TypeID = TMovieSceneAnimTypeID<FComponentAnimationActuator, 0>();
			return FMovieSceneBlendingActuatorID(TypeID);
		}

		static FMovieSceneAnimTypeID GetAnimControlTypeID()
		{
			static FMovieSceneAnimTypeID TypeID = TMovieSceneAnimTypeID<FComponentAnimationActuator, 2>();
			return TypeID;
		}

		virtual FBlendedAnimation RetrieveCurrentValue(UObject* InObject, IMovieScenePlayer* Player) const
		{
			check(false);
			return FBlendedAnimation();
		}

		virtual void Actuate(UObject* InObject, const FBlendedAnimation& InFinalValue, const TBlendableTokenStack<FBlendedAnimation>& OriginalStack, const FMovieSceneContext& Context, FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) override
		{
			ensureMsgf(InObject, TEXT("Attempting to evaluate an Animation track with a null object."));

			USkeletalMeshComponent* SkeletalMeshComponent = SkeletalMeshComponentFromObject(InObject);
			if (!SkeletalMeshComponent)
			{
				return;
			}

			OriginalStack.SavePreAnimatedState(Player, *SkeletalMeshComponent, GetAnimControlTypeID(), FPreAnimatedAnimationTokenProducer());

			UAnimCustomInstance::BindToSkeletalMeshComponent<UAnimSequencerInstance>(SkeletalMeshComponent);

			const bool bPreviewPlayback = ShouldUsePreviewPlayback(Player, *SkeletalMeshComponent);

			const EMovieScenePlayerStatus::Type PlayerStatus = Player.GetPlaybackStatus();

			// If the playback status is jumping, ie. one such occurrence is setting the time for thumbnail generation, disable anim notifies updates because it could fire audio
			const bool bFireNotifies = !bPreviewPlayback || (PlayerStatus != EMovieScenePlayerStatus::Jumping && PlayerStatus != EMovieScenePlayerStatus::Stopped);

			// When jumping from one cut to another cut, the delta time should be 0 so that anim notifies before the current position are not evaluated. Note, anim notifies at the current time should still be evaluated.
			const float DeltaTime = Context.HasJumped() ? 0.f : Context.GetRange().Size<float>();

			const bool bResetDynamics = PlayerStatus == EMovieScenePlayerStatus::Stepping || 
										PlayerStatus == EMovieScenePlayerStatus::Jumping || 
										PlayerStatus == EMovieScenePlayerStatus::Scrubbing || 
										(DeltaTime == 0.0f && PlayerStatus != EMovieScenePlayerStatus::Stopped); 
		
			static const bool bLooping = false;
			for (const FMinimalAnimParameters& AnimParams : InFinalValue.AllAnimations)
			{
				Player.PreAnimatedState.SetCaptureEntity(AnimParams.EvaluationScope.Key, AnimParams.EvaluationScope.CompletionMode);

				if (bPreviewPlayback)
				{
					PreviewSetAnimPosition(PersistentData, Player, SkeletalMeshComponent,
						AnimParams.SlotName, AnimParams.Section, AnimParams.Animation, AnimParams.EvalTime, AnimParams.BlendWeight,
						bLooping, bFireNotifies, DeltaTime, Player.GetPlaybackStatus() == EMovieScenePlayerStatus::Playing, bResetDynamics);
				}
				else
				{
					SetAnimPosition(PersistentData, Player, SkeletalMeshComponent,
						AnimParams.SlotName, AnimParams.Section, AnimParams.Animation, AnimParams.EvalTime, AnimParams.BlendWeight,
						bLooping, bFireNotifies);
				}
			}

			Player.PreAnimatedState.SetCaptureEntity(FMovieSceneEvaluationKey(), EMovieSceneCompletionMode::KeepState);
		}

	private:

		static USkeletalMeshComponent* SkeletalMeshComponentFromObject(UObject* InObject)
		{
			USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(InObject);
			if (SkeletalMeshComponent)
			{
				return SkeletalMeshComponent;
			}
			// then check to see if we are controlling an actor & if so use its first USkeletalMeshComponent 
			else if (AActor* Actor = Cast<AActor>(InObject))
			{
				return Actor->FindComponentByClass<USkeletalMeshComponent>();
			}
			return nullptr;
		}

		void SetAnimPosition(FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player, USkeletalMeshComponent* SkeletalMeshComponent, FName SlotName, FObjectKey Section, UAnimSequenceBase* InAnimSequence, float InPosition, float Weight, bool bLooping, bool bFireNotifies)
		{
			if (!CanPlayAnimation(SkeletalMeshComponent, InAnimSequence))
			{
				return;
			}

			UAnimSequencerInstance* SequencerInst = Cast<UAnimSequencerInstance>(SkeletalMeshComponent->GetAnimInstance());
			if (SequencerInst)
			{
				FMovieSceneAnimTypeID AnimTypeID = SectionToAnimationIDs.GetAnimTypeID(Section);

				Player.SavePreAnimatedState(*SequencerInst, AnimTypeID, FStatelessPreAnimatedTokenProducer(&ResetAnimSequencerInstance));

				// Set position and weight
				SequencerInst->UpdateAnimTrack(InAnimSequence, GetTypeHash(AnimTypeID), InPosition, Weight, bFireNotifies);
			}
			else if (UAnimInstance* AnimInst = SkeletalMeshComponent->GetAnimInstance())
			{
				FMontagePlayerPerSectionData* SectionData = MontageData.Find(Section);

				int32 InstanceId = (SectionData) ? SectionData->MontageInstanceId : INDEX_NONE;
				TWeakObjectPtr<UAnimMontage> Montage = FAnimMontageInstance::SetSequencerMontagePosition(SlotName, SkeletalMeshComponent, InstanceId, InAnimSequence, InPosition, Weight, bLooping);

				if (Montage.IsValid())
				{
					FMontagePlayerPerSectionData& DataContainer = MontageData.FindOrAdd(Section);
					DataContainer.Montage = Montage;
					DataContainer.MontageInstanceId = InstanceId;

					FMovieSceneAnimTypeID SlotTypeID = SectionToAnimationIDs.GetAnimTypeID(Section);
					Player.SavePreAnimatedState(*Montage.Get(), SlotTypeID, FStopPlayingMontageTokenProducer(AnimInst, InstanceId));

					// make sure it's playing
					FAnimMontageInstance* Instance = AnimInst->GetMontageInstanceForID(InstanceId);
					Instance->bPlaying = true;
				}
			}
		}

		void PreviewSetAnimPosition(FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player, USkeletalMeshComponent* SkeletalMeshComponent, FName SlotName, FObjectKey Section, UAnimSequenceBase* InAnimSequence, float InPosition, float Weight, bool bLooping, bool bFireNotifies, float DeltaTime, bool bPlaying, bool bResetDynamics)
		{
			if (!CanPlayAnimation(SkeletalMeshComponent, InAnimSequence))
			{
				return;
			}

			UAnimSequencerInstance* SequencerInst = Cast<UAnimSequencerInstance>(SkeletalMeshComponent->GetAnimInstance());
			if (SequencerInst)
			{
				// Unique anim type ID per slot
				FMovieSceneAnimTypeID AnimTypeID = SectionToAnimationIDs.GetAnimTypeID(Section);
				Player.SavePreAnimatedState(*SequencerInst, AnimTypeID, FStatelessPreAnimatedTokenProducer(&ResetAnimSequencerInstance));

				// Set position and weight
				SequencerInst->UpdateAnimTrack(InAnimSequence, GetTypeHash(AnimTypeID), InPosition, Weight, bFireNotifies);
			}
			else if (UAnimInstance* AnimInst = SkeletalMeshComponent->GetAnimInstance())
			{
				FMontagePlayerPerSectionData* SectionData = MontageData.Find(Section);

				int32 InstanceId = (SectionData)? SectionData->MontageInstanceId : INDEX_NONE;
				TWeakObjectPtr<UAnimMontage> Montage = FAnimMontageInstance::PreviewSequencerMontagePosition(SlotName, SkeletalMeshComponent, InstanceId, InAnimSequence, InPosition, Weight, bLooping, bFireNotifies);

				if (Montage.IsValid())
				{
					FMontagePlayerPerSectionData& DataContainer = MontageData.FindOrAdd(Section);
					DataContainer.Montage = Montage;
					DataContainer.MontageInstanceId = InstanceId;

					FMovieSceneAnimTypeID AnimTypeID = SectionToAnimationIDs.GetAnimTypeID(InAnimSequence);
					Player.SavePreAnimatedState(*Montage.Get(), AnimTypeID, FStopPlayingMontageTokenProducer(AnimInst, InstanceId));

					FAnimMontageInstance* Instance = AnimInst->GetMontageInstanceForID(InstanceId);
					Instance->bPlaying = bPlaying;
				}
	
				if (bResetDynamics)
				{
					// make sure we reset any simulations
					AnimInst->ResetDynamics();
				}
			}
		}

		TMovieSceneAnimTypeIDContainer<FObjectKey> SectionToAnimationIDs;
		TMap<FObjectKey, FMontagePlayerPerSectionData> MontageData;
	};

}	// namespace MovieScene

template<> FMovieSceneAnimTypeID GetBlendingDataType<MovieScene::FBlendedAnimation>()
{
	static FMovieSceneAnimTypeID TypeID = FMovieSceneAnimTypeID::Unique();
	return TypeID;
}

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
		float Weight = Params.Weight.Eval(Context.GetTime()) * EvaluateEasing(Context.GetTime());

		FOptionalMovieSceneBlendType BlendType = SourceSection->GetBlendType();
		check(BlendType.IsValid());

		// Ensure the accumulator knows how to actually apply component transforms
		FMovieSceneBlendingActuatorID ActuatorTypeID = MovieScene::FComponentAnimationActuator::GetActuatorTypeID();
		FMovieSceneBlendingAccumulator& Accumulator = ExecutionTokens.GetBlendingAccumulator();
		if (!Accumulator.FindActuator<MovieScene::FBlendedAnimation>(ActuatorTypeID))
		{
			Accumulator.DefineActuator(ActuatorTypeID, MakeShared<MovieScene::FComponentAnimationActuator>());
		}

		// Add the blendable to the accumulator
		FMinimalAnimParameters AnimParams(
			Params.Animation, EvalTime, Weight, ExecutionTokens.GetCurrentScope(), Params.SlotName, GetSourceSection()
			);
		ExecutionTokens.BlendToken(ActuatorTypeID, TBlendableToken<MovieScene::FBlendedAnimation>(AnimParams, BlendType.Get(), 1.f));
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