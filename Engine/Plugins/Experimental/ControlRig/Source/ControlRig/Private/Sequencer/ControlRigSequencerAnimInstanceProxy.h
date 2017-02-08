// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AnimSequencerInstanceProxy.h"
#include "AnimNode_ControlRig.h"
#include "Animation/AnimNodeSpaceConversions.h"
#include "ControlRigSequencerAnimInstanceProxy.generated.h"

struct FSequencerPlayerControlRig : public FSequencerPlayerBase
{
	SEQUENCER_INSTANCE_PLAYER_TYPE(FSequencerPlayerControlRig, FSequencerPlayerBase)

	FAnimNode_ConvertComponentToLocalSpace ConversionNode;
	FAnimNode_ControlRig ControlRigNode;
};

/** Proxy that manages adding animation ControlRig nodes as well as acting as a regular sequencer proxy */
USTRUCT()
struct CONTROLRIG_API FControlRigSequencerAnimInstanceProxy : public FAnimSequencerInstanceProxy
{
	GENERATED_BODY()

public:
	FControlRigSequencerAnimInstanceProxy()
	{
	}

	FControlRigSequencerAnimInstanceProxy(UAnimInstance* InAnimInstance)
		: FAnimSequencerInstanceProxy(InAnimInstance)
	{
	}

	// AnimInstanceProxy interface
	virtual void CacheBones() override;

	bool UpdateControlRig(UControlRig* InControlRig, uint32 SequenceId, bool bAdditive, float Weight);

private:
	void InitControlRigTrack(UControlRig* InControlRig, bool bAdditive, uint32 SequenceId);
	bool EnsureControlRigTrack(UControlRig* InControlRig, bool bAdditive, uint32 SequenceId);
};