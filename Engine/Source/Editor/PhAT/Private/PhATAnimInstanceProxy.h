// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AnimInstanceProxy.h"
//#include "AnimNode_RigidBody.h"
#include "Animation/AnimNodeSpaceConversions.h"
#include "Animation/AnimNode_SequencePlayer.h"
#include "PhATAnimInstanceProxy.generated.h"

class UAnimSequence;

/** Proxy override for this UAnimInstance-derived class */
USTRUCT()
struct FPhATAnimInstanceProxy : public FAnimInstanceProxy
{
	GENERATED_BODY()

public:
	FPhATAnimInstanceProxy()
	{
	}

	FPhATAnimInstanceProxy(UAnimInstance* InAnimInstance)
		: FAnimInstanceProxy(InAnimInstance)
	{
	}

	virtual ~FPhATAnimInstanceProxy();

	virtual void Initialize(UAnimInstance* InAnimInstance) override;
	virtual FAnimNode_Base* GetCustomRootNode() override
	{
		return &ComponentToLocalSpace;
	}

	bool IsPlayingAnimation() const;
	void SetAnimationSequence(UAnimSequence* AnimSequence);
	void SetPlayRate(float PlayRate);

	virtual void GetCustomNodes(TArray<FAnimNode_Base*>& OutNodes) override;
private:
	void ConstructNodes();

	//FAnimNode_RigidBody RagdollNode;
	FAnimNode_ConvertComponentToLocalSpace ComponentToLocalSpace;
	FAnimNode_SequencePlayer SequencePlayerNode;
	FAnimNode_ConvertLocalToComponentSpace LocalToComponentSpace;
	
	void InitAnimTrack(UAnimSequenceBase* InAnimSequence, int32 SequenceId);
	void EnsureAnimTrack(UAnimSequenceBase* InAnimSequence, int32 SequenceId);
};