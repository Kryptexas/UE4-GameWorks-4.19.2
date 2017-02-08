// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "ControlRigBindingTemplate.h"
#include "MovieSceneSequence.h"
#include "Sections/MovieSceneSpawnSection.h"
#include "MovieSceneEvaluation.h"
#include "IMovieScenePlayer.h"
#include "ControlRig.h"
#include "Components/SkeletalMeshComponent.h"
#include "ControlRigSequencerAnimInstance.h"

DECLARE_CYCLE_STAT(TEXT("Binding Track Evaluate"), MovieSceneEval_BindControlRigTemplate_Evaluate, STATGROUP_MovieSceneEval);
DECLARE_CYCLE_STAT(TEXT("Binding Track Token Execute"), MovieSceneEval_BindControlRig_TokenExecute, STATGROUP_MovieSceneEval);

struct FControlRigPreAnimatedTokenProducer : IMovieScenePreAnimatedTokenProducer
{
	virtual IMovieScenePreAnimatedTokenPtr CacheExistingState(UObject& Object) const
	{
		struct FToken : IMovieScenePreAnimatedToken
		{
			virtual void RestoreState(UObject& InObject, IMovieScenePlayer& Player) override
			{
				Player.GetSpawnRegister().DestroyObjectDirectly(InObject);
			}
		};

		return FToken();
	}
};

struct FBindControlRigObjectToken : IMovieSceneExecutionToken
{
	FBindControlRigObjectToken(FGuid InObjectBindingId, FMovieSceneSequenceIDRef InObjectBindingSequenceID, float InWeight, bool bInSpawned)
		: 
#if WITH_EDITORONLY_DATA
		ObjectBinding(nullptr),
#endif
		ObjectBindingId(InObjectBindingId)
		, ObjectBindingSequenceID(InObjectBindingSequenceID)
		, Weight(InWeight)
		, bSpawned(bInSpawned)
	{}

#if WITH_EDITORONLY_DATA
	FBindControlRigObjectToken(TWeakObjectPtr<> InObjectBinding, float InWeight, bool bInSpawned)
		: ObjectBinding(InObjectBinding)
		, Weight(InWeight)
		, bSpawned(bInSpawned)
	{
		ObjectBindingId.Invalidate();
	}
#endif

	virtual void Execute(const FMovieSceneContext& Context, const FMovieSceneEvaluationOperand& Operand, FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) override
	{
		MOVIESCENE_DETAILED_SCOPE_CYCLE_COUNTER(MovieSceneEval_BindControlRig_TokenExecute)

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
					if (ObjectBindingId.IsValid())
					{
						TArrayView<TWeakObjectPtr<>> OuterBoundObjects = Player.FindBoundObjects(ObjectBindingId, ObjectBindingSequenceID);
						if (OuterBoundObjects.Num() > 0)
						{
							ControlRig->BindToObject(OuterBoundObjects[0].Get());
						}
					}
#if WITH_EDITORONLY_DATA
					else if (ObjectBinding.IsValid())
					{
						ControlRig->BindToObject(ObjectBinding.Get());
					}
#endif

					if (USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(ControlRig->GetBoundObject()))
					{
						if (UControlRigSequencerAnimInstance* AnimInstance = UAnimSequencerInstance::BindToSkeletalMeshComponent<UControlRigSequencerAnimInstance>(SkeletalMeshComponent))
						{
							AnimInstance->RecalcRequiredBones();
						}
					}
				}
			}
			else
			{
				ControlRig = Cast<UControlRig>(BoundObjects[0].Get());
			}

			// Update the animation's state
			if (ControlRig)
			{
				if (USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(ControlRig->GetBoundObject()))
				{
					if (UControlRigSequencerAnimInstance* AnimInstance = Cast<UControlRigSequencerAnimInstance>(SkeletalMeshComponent->GetAnimInstance()))
					{
						bool bStructureChanged = AnimInstance->UpdateControlRig(ControlRig, Operand.SequenceID.GetInternalValue(), /*bAdditive*/ false, Weight);
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
					Player.SavePreAnimatedState(*ObjectPtr, FControlRigBindingTemplate::GetAnimTypeID(), FControlRigPreAnimatedTokenProducer());
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
							AnimInstance->UpdateControlRig(ControlRig, Operand.SequenceID.GetInternalValue(), /*bAdditive*/ false, 0.0f);
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

	/** The object binding controllers should bind to (in the case we are bound to a sequencer object) */
	FGuid ObjectBindingId;

	/** The sequence ID controllers should bind to */
	FMovieSceneSequenceID ObjectBindingSequenceID;

	/** The weight to apply this controller at */
	float Weight;

	/** Whether this token should spawn an object */
	bool bSpawned;
};

#if WITH_EDITORONLY_DATA
FControlRigBindingTemplate::FControlRigBindingTemplate(const UMovieSceneSpawnSection& SpawnSection, TWeakObjectPtr<> InObjectBinding)
	: FMovieSceneSpawnSectionTemplate(SpawnSection)
	, ObjectBinding(InObjectBinding)
{
	ObjectBindingId.Invalidate();
}
#endif

FControlRigBindingTemplate::FControlRigBindingTemplate(const UMovieSceneSpawnSection& SpawnSection, FGuid InObjectBindingId, FMovieSceneSequenceIDRef InObjectBindingSequenceID)
	: FMovieSceneSpawnSectionTemplate(SpawnSection)
#if WITH_EDITORONLY_DATA
	, ObjectBinding(nullptr)
#endif
	, ObjectBindingId(InObjectBindingId)
	, ObjectBindingSequenceID(InObjectBindingSequenceID)
{
}

void FControlRigBindingTemplate::Evaluate(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const
{
	MOVIESCENE_DETAILED_SCOPE_CYCLE_COUNTER(MovieSceneEval_BindControlRigTemplate_Evaluate)

	const float Time = Context.GetTime();

	if (ObjectBindingId.IsValid())
	{
		ExecutionTokens.Add(FBindControlRigObjectToken(ObjectBindingId, ObjectBindingSequenceID, 1.0f, Curve.Evaluate(Time) != 0));
	}
#if WITH_EDITORONLY_DATA
	else if (ObjectBinding.IsValid())
	{
		ExecutionTokens.Add(FBindControlRigObjectToken(ObjectBinding, 1.0f, Curve.Evaluate(Time) != 0));
	}
#endif
}

FMovieSceneAnimTypeID FControlRigBindingTemplate::GetAnimTypeID()
{
	return TMovieSceneAnimTypeID<FControlRigBindingTemplate>();
}

