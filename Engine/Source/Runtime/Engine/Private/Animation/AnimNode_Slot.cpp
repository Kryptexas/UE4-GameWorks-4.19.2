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


void FAnimNode_Slot::GatherDebugData(FNodeDebugData& DebugData)
{
	FString DebugLine = DebugData.GetNodeName(this);
	DebugLine += FString::Printf(TEXT("(Slot Name: '%s' Weight:%.1f%%)"), *SlotName.ToString(), SlotNodeWeight*100.f);

	bool bIsPoseSource = !bNeedToEvaluateSource;
	DebugData.AddDebugItem(DebugLine, bIsPoseSource);
	Source.GatherDebugData(DebugData.BranchFlow(bNeedToEvaluateSource ? 1.f : 0.f));

	for (FAnimMontageInstance* MontageInstance : DebugData.AnimInstance->MontageInstances)
	{
		if (MontageInstance->IsValid() && MontageInstance->Montage->IsValidSlot(SlotName))
		{
			const FAnimTrack* Track = MontageInstance->Montage->GetAnimationData(SlotName);
			for (const FAnimSegment& Segment : Track->AnimSegments)
			{
				float Weight;
				float CurrentAnimPos;
				if (UAnimSequenceBase* Anim = Segment.GetAnimationData(MontageInstance->Position, CurrentAnimPos, Weight))
				{
					FString MontageLine = FString::Printf(TEXT("Montage: '%s' Anim: '%s' Play Time:%.2f W:%.2f%%"), *MontageInstance->Montage->GetName(), *Anim->GetName(), CurrentAnimPos, Weight*100.f);
					DebugData.BranchFlow(1.0f).AddDebugItem(MontageLine, true);
					break;
				}
			}
		}
	}
}

FAnimNode_Slot::FAnimNode_Slot()
	: bNeedToEvaluateSource(false)
{
}
