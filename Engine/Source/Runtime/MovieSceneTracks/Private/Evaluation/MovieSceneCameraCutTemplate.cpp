// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "Evaluation/MovieSceneCameraCutTemplate.h"
#include "Sections/MovieSceneCameraCutSection.h"
#include "MovieSceneEvaluation.h"
#include "IMovieScenePlayer.h"
#include "GameFramework/Actor.h"
#include "ContentStreaming.h"

DECLARE_CYCLE_STAT(TEXT("Camera Cut Track Token Execute"), MovieSceneEval_CameraCutTrack_TokenExecute, STATGROUP_MovieSceneEval);

/** A movie scene execution token that sets up the streaming system with the camera cut location */
struct FCameraCutPreRollExecutionToken : IMovieSceneExecutionToken
{
	FMovieSceneObjectBindingID CameraBindingID;
	FTransform CutTransform;
	bool bHasCutTransform;

	FCameraCutPreRollExecutionToken(const FMovieSceneObjectBindingID& InCameraBindingID, const FTransform& InCutTransform, bool bInHasCutTransform)
		: CameraBindingID(InCameraBindingID)
		, CutTransform(InCutTransform)
		, bHasCutTransform(bInHasCutTransform)
	{}

	static FMovieSceneAnimTypeID GetAnimTypeID()
	{
		return TMovieSceneAnimTypeID<FCameraCutPreRollExecutionToken>();
	}
	
	/** Execute this token, operating on all objects referenced by 'Operand' */
	virtual void Execute(const FMovieSceneContext& Context, const FMovieSceneEvaluationOperand& Operand, FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) override
	{
		FVector Location;

		if (bHasCutTransform)
		{
			Location = CutTransform.GetLocation();
		}
		else
		{
			FMovieSceneSequenceID SequenceID = Operand.SequenceID;
			if (CameraBindingID.GetSequenceID().IsValid())
			{
				if (const FMovieSceneSubSequenceData* SubData = Player.GetEvaluationTemplate().GetHierarchy().FindSubData(SequenceID))
				{
					// Ensure that this ID is resolvable from the root, based on the current local sequence ID
					FMovieSceneObjectBindingID RootBindingID = CameraBindingID.ResolveLocalToRoot(SequenceID, Player.GetEvaluationTemplate().GetHierarchy());
					SequenceID = RootBindingID.GetSequenceID();
				}
			}

			// If the transform is set, otherwise use the bound actor's transform
			FMovieSceneEvaluationOperand CameraOperand(SequenceID, CameraBindingID.GetGuid());
		
			TArrayView<TWeakObjectPtr<>> Objects = Player.FindBoundObjects(CameraOperand);
			if (!Objects.Num())
			{
				return;
			}

			// Only ever deal with one camera
			UObject* CameraObject = Objects[0].Get();
			
			AActor* Actor = Cast<AActor>(CameraObject);
			if (Actor)
			{
				Location = Actor->GetActorLocation();
			}
		}

		IStreamingManager::Get().AddViewSlaveLocation( Location );
	}
};

struct FCameraCutTrackData : IPersistentEvaluationData
{
	TWeakObjectPtr<> LastLockedCamera;
};

/** A movie scene pre-animated token that stores a pre-animated camera cut */
struct FCameraCutPreAnimatedToken : IMovieScenePreAnimatedGlobalToken
{
	virtual void RestoreState(IMovieScenePlayer& Player) override
	{
		Player.UpdateCameraCut(nullptr, nullptr);
	}
};

/** A movie scene execution token that applies camera cuts */
struct FCameraCutExecutionToken : IMovieSceneExecutionToken
{
	FMovieSceneObjectBindingID CameraBindingID;

	FCameraCutExecutionToken(const FMovieSceneObjectBindingID& InCameraBindingID)
		: CameraBindingID(InCameraBindingID)
	{}

	static FMovieSceneAnimTypeID GetAnimTypeID()
	{
		return TMovieSceneAnimTypeID<FCameraCutExecutionToken>();
	}
	
	/** Execute this token, operating on all objects referenced by 'Operand' */
	virtual void Execute(const FMovieSceneContext& Context, const FMovieSceneEvaluationOperand& Operand, FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) override
	{
		if (!Player.CanUpdateCameraCut())
		{
			Player.UpdateCameraCut(nullptr, nullptr);
			return;
		}

		MOVIESCENE_DETAILED_SCOPE_CYCLE_COUNTER(MovieSceneEval_CameraCutTrack_TokenExecute)

		FMovieSceneSequenceID SequenceID = Operand.SequenceID;
		if (CameraBindingID.GetSequenceID().IsValid())
		{
			if (const FMovieSceneSubSequenceData* SubData = Player.GetEvaluationTemplate().GetHierarchy().FindSubData(SequenceID))
			{
				// Ensure that this ID is resolvable from the root, based on the current local sequence ID
				FMovieSceneObjectBindingID RootBindingID = CameraBindingID.ResolveLocalToRoot(SequenceID, Player.GetEvaluationTemplate().GetHierarchy());
				SequenceID = RootBindingID.GetSequenceID();
			}
		}

		FMovieSceneEvaluationOperand CameraOperand(SequenceID, CameraBindingID.GetGuid());
		
		TArrayView<TWeakObjectPtr<>> Objects = Player.FindBoundObjects(CameraOperand);
		if (!Objects.Num())
		{
			return;
		}

		// Only ever deal with one camera
		UObject* CameraObject = Objects[0].Get();

		struct FProducer : IMovieScenePreAnimatedGlobalTokenProducer
		{
			virtual IMovieScenePreAnimatedGlobalTokenPtr CacheExistingState() const override
			{
				return FCameraCutPreAnimatedToken();
			}
		} Producer;

		Player.SavePreAnimatedState(GetAnimTypeID(), Producer);

		FCameraCutTrackData& CameraCutCache = PersistentData.GetOrAddTrackData<FCameraCutTrackData>();
		if (CameraCutCache.LastLockedCamera.Get() != CameraObject)
		{
			Player.UpdateCameraCut(CameraObject, CameraCutCache.LastLockedCamera.Get(), Context.HasJumped());
			CameraCutCache.LastLockedCamera = CameraObject;
		}
		else if (CameraObject)
		{
			Player.UpdateCameraCut(CameraObject, nullptr, Context.HasJumped());
		}
	}
};


FMovieSceneCameraCutSectionTemplate::FMovieSceneCameraCutSectionTemplate(const UMovieSceneCameraCutSection& Section, TOptional<FTransform> InCutTransform)
	: CameraBindingID(Section.GetCameraBindingID())
	, CutTransform(InCutTransform.Get(FTransform()))
	, bHasCutTransform(InCutTransform.IsSet())
{
}

void FMovieSceneCameraCutSectionTemplate::Evaluate(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const
{
	if (Context.IsPreRoll())
	{
		ExecutionTokens.Add(FCameraCutPreRollExecutionToken(CameraBindingID, CutTransform, bHasCutTransform));
	}
	else
	{
		ExecutionTokens.Add(FCameraCutExecutionToken(CameraBindingID));
	}
}
