// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "ControlRigSequencerAnimInstanceProxy.h"
#include "ControlRigSequencerAnimInstance.h"

void FControlRigSequencerAnimInstanceProxy::CacheBones()
{
	// As we dont use the RootNode (this is not using anim blueprint), 
	// we just cache the nodes we know about
	if (bBoneCachesInvalidated)
	{
		bBoneCachesInvalidated = false;

		CachedBonesCounter.Increment();
		FAnimationCacheBonesContext Proxy(this);
		SequencerRootNode.CacheBones(Proxy);
	}
}

void FControlRigSequencerAnimInstanceProxy::InitControlRigTrack(UControlRig* InControlRig, bool bAdditive, uint32 SequenceId)
{
	if (InControlRig != nullptr)
	{
		FSequencerPlayerControlRig* PlayerState = FindPlayer<FSequencerPlayerControlRig>(SequenceId);
		if (PlayerState == nullptr)
		{
			FAnimNode_MultiWayBlend& BlendNode = bAdditive ? AdditiveBlendNode : FullBodyBlendNode;

			const int32 PoseIndex = BlendNode.AddPose() - 1;

			// add the new entry to map
			FSequencerPlayerControlRig* NewPlayerState = new FSequencerPlayerControlRig();
			NewPlayerState->PoseIndex = PoseIndex;
			NewPlayerState->bAdditive = bAdditive;

			SequencerToPlayerMap.Add(SequenceId, NewPlayerState);

			// link ControlRig node & space conversion to blendnode
			BlendNode.Poses[PoseIndex].SetLinkNode(&NewPlayerState->ConversionNode);
			NewPlayerState->ConversionNode.ComponentPose.SetLinkNode(&NewPlayerState->ControlRigNode);

			// set player state
			PlayerState = NewPlayerState;
		}

		// now set animation data to player
		PlayerState->ControlRigNode.SetControlRig(InControlRig);

		// initialize player
		PlayerState->ControlRigNode.RootInitialize(this);
		PlayerState->ControlRigNode.Initialize(FAnimationInitializeContext(this));
	}
}

bool FControlRigSequencerAnimInstanceProxy::UpdateControlRig(UControlRig* InControlRig, uint32 SequenceId, bool bAdditive, float Weight)
{
	bool bCreated = EnsureControlRigTrack(InControlRig, bAdditive, SequenceId);

	FSequencerPlayerControlRig* PlayerState = FindPlayer<FSequencerPlayerControlRig>(SequenceId);
	FAnimNode_MultiWayBlend& BlendNode = bAdditive ? AdditiveBlendNode : FullBodyBlendNode;
	BlendNode.DesiredAlphas[PlayerState->PoseIndex] = Weight;

	return bCreated;
}

bool FControlRigSequencerAnimInstanceProxy::EnsureControlRigTrack(UControlRig* InControlRig, bool bAdditive, uint32 SequenceId)
{
	FSequencerPlayerControlRig* PlayerState = FindPlayer<FSequencerPlayerControlRig>(SequenceId);
	if (!PlayerState || InControlRig != PlayerState->ControlRigNode.GetControlRig())
	{
		InitControlRigTrack(InControlRig, bAdditive, SequenceId);
		return true;
	}

	return false;
}