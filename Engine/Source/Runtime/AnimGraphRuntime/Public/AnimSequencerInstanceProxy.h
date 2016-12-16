// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AnimInstanceProxy.h"
#include "AnimNodes/AnimNode_SequenceEvaluator.h"
#include "AnimNodes/AnimNode_ApplyAdditive.h"
#include "AnimNodes/AnimNode_MultiWayBlend.h"
#include "AnimSequencerInstanceProxy.generated.h"

struct FSequencerPlayerState
{
	int32 PoseIndex;
	bool bAdditive;
	struct FAnimNode_SequenceEvaluator PlayerNode;

	FSequencerPlayerState()
		: PoseIndex(INDEX_NONE)
		, bAdditive(false)
	{}
};

/** Proxy override for this UAnimInstance-derived class */
USTRUCT()
struct ANIMGRAPHRUNTIME_API FAnimSequencerInstanceProxy : public FAnimInstanceProxy
{
	GENERATED_BODY()

public:
	FAnimSequencerInstanceProxy()
	{
	}

	FAnimSequencerInstanceProxy(UAnimInstance* InAnimInstance)
		: FAnimInstanceProxy(InAnimInstance)
	{
	}

	virtual ~FAnimSequencerInstanceProxy();

	virtual void Initialize(UAnimInstance* InAnimInstance) override;
	virtual bool Evaluate(FPoseContext& Output) override;
	virtual void UpdateAnimationNode(float DeltaSeconds) override;

	void UpdateAnimTrack(UAnimSequenceBase* InAnimSequence, int32 SequenceId, float InPosition, float Weight, bool bFireNotifies);
	void ResetNodes();
private:
	void ConstructNodes();
	struct FSequencerPlayerState* FindPlayer(int32 SequenceId)
	{
		return SequencerToPlayerMap.FindRef(SequenceId);
	}

	/** custom root node for this sequencer player. Didn't want to use RootNode in AnimInstance because it's used with lots of AnimBP functionality */
	struct FAnimNode_ApplyAdditive SequencerRootNode;
	struct FAnimNode_MultiWayBlend FullBodyBlendNode;
	struct FAnimNode_MultiWayBlend AdditiveBlendNode;

	/** mapping from sequencer index to internal player index */
	TMap<int32, FSequencerPlayerState*> SequencerToPlayerMap;

	void InitAnimTrack(UAnimSequenceBase* InAnimSequence, int32 SequenceId);
	void EnsureAnimTrack(UAnimSequenceBase* InAnimSequence, int32 SequenceId);
	void ClearSequencePlayerMap();
};