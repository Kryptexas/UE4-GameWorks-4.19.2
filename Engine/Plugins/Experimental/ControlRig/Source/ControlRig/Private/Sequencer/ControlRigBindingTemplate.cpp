// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "ControlRigBindingTemplate.h"
#include "MovieSceneSequence.h"
#include "Sections/MovieSceneSpawnSection.h"
#include "MovieSceneEvaluation.h"
#include "IMovieScenePlayer.h"
#include "ControlRig.h"
#include "Components/SkeletalMeshComponent.h"
#include "ControlRigSequencerAnimInstance.h"
#include "MovieSceneControlRigInstanceData.h"

DECLARE_CYCLE_STAT(TEXT("Binding Track Evaluate"), MovieSceneEval_BindControlRigTemplate_Evaluate, STATGROUP_MovieSceneEval);
DECLARE_CYCLE_STAT(TEXT("Binding Track Token Execute"), MovieSceneEval_BindControlRig_TokenExecute, STATGROUP_MovieSceneEval);

#if WITH_EDITORONLY_DATA
TWeakObjectPtr<UObject> FControlRigBindingTemplate::ObjectBinding;
#endif

struct FControlRigPreAnimatedTokenProducer : IMovieScenePreAnimatedTokenProducer
{
	FControlRigPreAnimatedTokenProducer(FMovieSceneSequenceIDRef InSequenceID)
		: SequenceID(InSequenceID)
	{}

	virtual IMovieScenePreAnimatedTokenPtr CacheExistingState(UObject& Object) const
	{
		struct FToken : IMovieScenePreAnimatedToken
		{
			FToken(FMovieSceneSequenceIDRef InSequenceID)
				: SequenceID(InSequenceID)
			{}

			virtual void RestoreState(UObject& InObject, IMovieScenePlayer& Player) override
			{
				if (UControlRig* ControlRig = Cast<UControlRig>(&InObject))
				{
					if (USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(ControlRig->GetBoundObject()))
					{
						if (UControlRigSequencerAnimInstance* AnimInstance = Cast<UControlRigSequencerAnimInstance>(SkeletalMeshComponent->GetAnimInstance()))
						{
							// Force us to zero weight before we despawn, as the graph could persist
							AnimInstance->ResetNodes();
							AnimInstance->RecalcRequiredBones();
						}
						UAnimSequencerInstance::UnbindFromSkeletalMeshComponent(SkeletalMeshComponent);
					}

					ControlRig->UnbindFromObject();
				}

				Player.GetSpawnRegister().DestroyObjectDirectly(InObject);
			}

			FMovieSceneSequenceID SequenceID;
		};

		return FToken(SequenceID);
	}

	FMovieSceneSequenceID SequenceID;
};

struct FBindControlRigObjectToken : IMovieSceneExecutionToken
{

	FBindControlRigObjectToken(float InWeight, bool bInSpawned
#if WITH_EDITORONLY_DATA
		, TWeakObjectPtr<> InObjectBinding = TWeakObjectPtr<>()
#endif
		)
		: 
#if WITH_EDITORONLY_DATA
		  ObjectBinding(InObjectBinding),
#endif
		  Weight(InWeight)
		, bSpawned(bInSpawned)
	{}

	void BindToSequencerInstance(UControlRig* ControlRig)
	{
		check(ControlRig);
		if (USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(ControlRig->GetBoundObject()))
		{
			if (UControlRigSequencerAnimInstance* AnimInstance = UAnimCustomInstance::BindToSkeletalMeshComponent<UControlRigSequencerAnimInstance>(SkeletalMeshComponent))
			{
				AnimInstance->RecalcRequiredBones();
			}
		}
	}

	void UnBindFromSequencerInstance(UControlRig* ControlRig)
	{
		check(ControlRig);
		if (USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(ControlRig->GetBoundObject()))
		{
			UAnimCustomInstance::UnbindFromSkeletalMeshComponent(SkeletalMeshComponent);
		}
	}

	virtual void Execute(const FMovieSceneContext& Context, const FMovieSceneEvaluationOperand& Operand, FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) override
	{
		MOVIESCENE_DETAILED_SCOPE_CYCLE_COUNTER(MovieSceneEval_BindControlRig_TokenExecute)

		FInputBlendPose DefaultBoneFilter;

		FMovieSceneEvaluationOperand InstanceOperand;
		bool bAdditive = false;
		bool bApplyBoneFilter = false;
		const FInputBlendPose* BoneFilter = &DefaultBoneFilter;

		if (const FMovieSceneControlRigInstanceData* InstanceData = PersistentData.FindInstanceData<FMovieSceneControlRigInstanceData>())
		{
			InstanceOperand = InstanceData->Operand;
			bAdditive = InstanceData->bAdditive;
			bApplyBoneFilter = InstanceData->bApplyBoneFilter;
			BoneFilter = &InstanceData->BoneFilter;
		}

		TArrayView<TWeakObjectPtr<>> BoundObjects = Player.FindBoundObjects(Operand);
		const bool bHasBoundObjects = BoundObjects.Num() != 0;
		if (bSpawned)
		{
			UControlRig* ControlRig = nullptr;

			// If it's not spawned, spawn it
			if (!bHasBoundObjects)
			{
				const UMovieSceneSequence* Sequence = Player.State.FindSequence(Operand.SequenceID);
				if (Sequence)
				{
					ControlRig = CastChecked<UControlRig>(Player.GetSpawnRegister().SpawnObject(Operand.ObjectBindingID, *Sequence->GetMovieScene(), Operand.SequenceID, Player));
					if (InstanceOperand.ObjectBindingID.IsValid())
					{
						TArrayView<TWeakObjectPtr<>> OuterBoundObjects = Player.FindBoundObjects(InstanceOperand);
						if (OuterBoundObjects.Num() > 0)
						{
							UObject* OuterBoundObject = OuterBoundObjects[0].Get();
							if (OuterBoundObject && !ControlRig->IsBoundToObject(OuterBoundObject))
							{
								UnBindFromSequencerInstance(ControlRig);
								ControlRig->UnbindFromObject();
								ControlRig->BindToObject(OuterBoundObject);
							}
						}
					}
#if WITH_EDITORONLY_DATA
					else if (ObjectBinding.IsValid() && !ControlRig->IsBoundToObject(ObjectBinding.Get()))
					{
						UnBindFromSequencerInstance(ControlRig);
						ControlRig->UnbindFromObject();
						ControlRig->BindToObject(ObjectBinding.Get());
					}
#endif
					BindToSequencerInstance(ControlRig);
				}
			}
			else
			{
				ControlRig = Cast<UControlRig>(BoundObjects[0].Get());

#if WITH_EDITORONLY_DATA
				if (ObjectBinding.IsValid() && !ControlRig->IsBoundToObject(ObjectBinding.Get()))
				{
					UnBindFromSequencerInstance(ControlRig);
					ControlRig->UnbindFromObject();
					ControlRig->BindToObject(ObjectBinding.Get());
				}

				BindToSequencerInstance(ControlRig);
#endif
			}

			// Update the animation's state
			if (ControlRig)
			{
				if (USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(ControlRig->GetBoundObject()))
				{
					if (UControlRigSequencerAnimInstance* AnimInstance = Cast<UControlRigSequencerAnimInstance>(SkeletalMeshComponent->GetAnimInstance()))
					{
						bool bStructureChanged = AnimInstance->UpdateControlRig(ControlRig, Operand.SequenceID.GetInternalValue(), bAdditive, bApplyBoneFilter, *BoneFilter, Weight);
						if (bStructureChanged)
						{
							AnimInstance->RecalcRequiredBones();
						}
					}
				}
			}

			// ensure that pre animated state is saved
			for (TWeakObjectPtr<> Object : Player.FindBoundObjects(Operand))
			{
				if (UObject* ObjectPtr = Object.Get())
				{
					Player.SavePreAnimatedState(*ObjectPtr, FControlRigBindingTemplate::GetAnimTypeID(), FControlRigPreAnimatedTokenProducer(Operand.SequenceID));
				}
			}
		}
		else if (!bSpawned && bHasBoundObjects)
		{
			for (TWeakObjectPtr<> Object : Player.FindBoundObjects(Operand))
			{
				if (UControlRig* ControlRig = Cast<UControlRig>(Object.Get()))
				{
					if(USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(ControlRig->GetBoundObject()))
					{
						if (UControlRigSequencerAnimInstance* AnimInstance = Cast<UControlRigSequencerAnimInstance>(SkeletalMeshComponent->GetAnimInstance()))
						{
							// Force us to zero weight before we despawn, as the graph could persist
							AnimInstance->UpdateControlRig(ControlRig, Operand.SequenceID.GetInternalValue(), bAdditive, bApplyBoneFilter, *BoneFilter, 0.0f);
							AnimInstance->RecalcRequiredBones();
						}
						UAnimSequencerInstance::UnbindFromSkeletalMeshComponent(SkeletalMeshComponent);
					}

					ControlRig->UnbindFromObject();
				}
			}

			Player.GetSpawnRegister().DestroySpawnedObject(Operand.ObjectBindingID, Operand.SequenceID, Player);
		}
	}

#if WITH_EDITORONLY_DATA
	/** The object that spawned controllers should bind to (in the case we are bound to a non-sequencer object) */
	TWeakObjectPtr<> ObjectBinding;
#endif

	/** The weight to apply this controller at */
	float Weight;

	/** Whether this token should spawn an object */
	bool bSpawned;
};

FControlRigBindingTemplate::FControlRigBindingTemplate(const UMovieSceneSpawnSection& SpawnSection)
	: FMovieSceneSpawnSectionTemplate(SpawnSection)
{
}

#if WITH_EDITORONLY_DATA

void FControlRigBindingTemplate::SetObjectBinding(TWeakObjectPtr<> InObjectBinding)
{
	ObjectBinding = InObjectBinding;
}

UObject* FControlRigBindingTemplate::GetObjectBinding()
{
	return ObjectBinding.Get();
}

void FControlRigBindingTemplate::ClearObjectBinding()
{
	ObjectBinding = nullptr;
}

#endif


void FControlRigBindingTemplate::Evaluate(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const
{
	MOVIESCENE_DETAILED_SCOPE_CYCLE_COUNTER(MovieSceneEval_BindControlRigTemplate_Evaluate)

	const FMovieSceneControlRigInstanceData* InstanceData = PersistentData.FindInstanceData<FMovieSceneControlRigInstanceData>();

	const float Time = Context.GetTime();
	const float Weight = InstanceData ? InstanceData->Weight.Eval(Time) : 1.f;

	if (InstanceData && InstanceData->Operand.ObjectBindingID.IsValid())
	{
		ExecutionTokens.Add(FBindControlRigObjectToken(Weight, Curve.Evaluate(Time) != 0));
	}
#if WITH_EDITORONLY_DATA
	else if (ObjectBinding.IsValid())
	{
		ExecutionTokens.Add(FBindControlRigObjectToken(Weight, Curve.Evaluate(Time) != 0, ObjectBinding));
	}
#endif
}

FMovieSceneAnimTypeID FControlRigBindingTemplate::GetAnimTypeID()
{
	return TMovieSceneAnimTypeID<FControlRigBindingTemplate>();
}

