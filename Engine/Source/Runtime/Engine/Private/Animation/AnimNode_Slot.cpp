// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

/////////////////////////////////////////////////////
// FAnimNode_Slot

void FAnimNode_Slot::Initialize(const FAnimationInitializeContext& Context)
{
	Source.Initialize(Context);

	bNeedToEvaluateSource = true;
	SlotNodeWeight = 0.f;

	Context.AnimInstance->RegisterSlotNode(SlotName);
}

void FAnimNode_Slot::CacheBones(const FAnimationCacheBonesContext & Context) 
{
	Source.CacheBones(Context);
}

void FAnimNode_Slot::Update(const FAnimationUpdateContext& Context)
{
	//@TODO: ANIMREFACTOR: POST: NeedToTickChildren calls GetSlotWeight internally, and should just expose that result as an additional out
	// This can probably naturally fall out as code is moved out of AnimInstance into the nodes
	SlotNodeWeight = Context.AnimInstance->GetSlotWeight(SlotName);
	// make sure to update slot weight
	Context.AnimInstance->UpdateSlotNodeWeight(SlotName, SlotNodeWeight);

	if (Context.AnimInstance->NeedToTickChildren(SlotName, SlotNodeWeight))
	{
		Source.Update(Context.FractionalWeight(1.0f-SlotNodeWeight));

		bNeedToEvaluateSource = true;
	}
	else
	{
		bNeedToEvaluateSource = false;
	}
}

void FAnimNode_Slot::Evaluate(FPoseContext& Output)
{
	//@TODO: ANIMREFACTOR: POST: Avoid the allocation if we don't need the child
	FPoseContext SourceContext(Output);
	
	if (bNeedToEvaluateSource)
	{
		Source.Evaluate(SourceContext);
	}

	Output.AnimInstance->SlotEvaluatePose(SlotName, SourceContext.Pose, Output.Pose, SlotNodeWeight);
}

FAnimNode_Slot::FAnimNode_Slot()
	: bNeedToEvaluateSource(false)
{
}
