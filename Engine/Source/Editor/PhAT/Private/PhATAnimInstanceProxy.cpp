// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "PhATAnimInstanceProxy.h"
#include "PhATAnimInstance.h"
//#include "AnimNode_RigidBody.h"

void FPhATAnimInstanceProxy::Initialize(UAnimInstance* InAnimInstance)
{
	FAnimInstanceProxy::Initialize(InAnimInstance);
	ConstructNodes();
}

void FPhATAnimInstanceProxy::ConstructNodes()
{
	LocalToComponentSpace.LocalPose.SetLinkNode(&SequencePlayerNode);
	//RagdollNode.ComponentPose.SetLinkNode(&LocalToComponentSpace);
	//ComponentToLocalSpace.ComponentPose.SetLinkNode(&RagdollNode);
	ComponentToLocalSpace.ComponentPose.SetLinkNode(&LocalToComponentSpace);
	
	//RagdollNode.SimulationSpace = ESimulationSpace::WorldSpace;
}

void FPhATAnimInstanceProxy::GetCustomNodes(TArray<FAnimNode_Base*>& OutNodes)
{
	//OutNodes.Add(&RagdollNode);
}

bool FPhATAnimInstanceProxy::IsPlayingAnimation() const
{
	return SequencePlayerNode.Sequence != nullptr && SequencePlayerNode.PlayRate > 0.f;
}

void FPhATAnimInstanceProxy::SetAnimationSequence(UAnimSequence* AnimSequence)
{
	SequencePlayerNode.Sequence = AnimSequence;
}

void FPhATAnimInstanceProxy::SetPlayRate(float PlayRate)
{
	SequencePlayerNode.PlayRate = PlayRate;
}

FPhATAnimInstanceProxy::~FPhATAnimInstanceProxy()
{
}