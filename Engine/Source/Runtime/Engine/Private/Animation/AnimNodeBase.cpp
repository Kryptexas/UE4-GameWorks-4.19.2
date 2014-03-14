// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

/////////////////////////////////////////////////////
// FPoseLinkBase

void FPoseLinkBase::AttemptRelink(const FAnimationBaseContext& Context)
{
	// Do the linkage
	if ((LinkedNode == NULL) && (LinkID != INDEX_NONE))
	{
		UAnimBlueprintGeneratedClass* AnimBlueprintClass = Context.GetAnimBlueprintClass();
		check(AnimBlueprintClass);

		UProperty* LinkedProperty = AnimBlueprintClass->AnimNodeProperties[LinkID];
		void* LinkedNodePtr = LinkedProperty->ContainerPtrToValuePtr<void>(Context.AnimInstance);
		LinkedNode = (FAnimNode_Base*)LinkedNodePtr;
	}
}

void FPoseLinkBase::Initialize(const FAnimationInitializeContext& Context)
{
	checkf( !bProcessed, TEXT( "Initialize already in progress, circular link for AnimInstance [%s] Blueprint [%s]" ), \
		Context.AnimInstance ? *Context.AnimInstance->GetFullName() : TEXT( "None" ), Context.GetAnimBlueprintClass() ? *Context.GetAnimBlueprintClass()->GetFullName() : TEXT( "None" ) );
	TGuardValue<bool> CircularGuard(bProcessed, true);

	AttemptRelink(Context);

	// Do standard initialization
	if (LinkedNode != NULL)
	{
		LinkedNode->Initialize(Context);
	}
}

void FPoseLinkBase::CacheBones(const FAnimationCacheBonesContext & Context) 
{
	checkf( !bProcessed, TEXT( "Initialize already in progress, circular link for AnimInstance [%s] Blueprint [%s]" ), \
		Context.AnimInstance ? *Context.AnimInstance->GetFullName() : TEXT( "None" ), Context.GetAnimBlueprintClass() ? *Context.GetAnimBlueprintClass()->GetFullName() : TEXT( "None" ) );
	TGuardValue<bool> CircularGuard(bProcessed, true);

	if (LinkedNode != NULL)
	{
		LinkedNode->CacheBones(Context);
	}
}

void FPoseLinkBase::Update(const FAnimationUpdateContext& Context)
{
	checkf( !bProcessed, TEXT( "Update already in progress, circular link for AnimInstance [%s] Blueprint [%s]" ), \
		Context.AnimInstance ? *Context.AnimInstance->GetFullName() : TEXT( "None" ), Context.GetAnimBlueprintClass() ? *Context.GetAnimBlueprintClass()->GetFullName() : TEXT( "None" ) );
	TGuardValue<bool> CircularGuard(bProcessed, true);

#if WITH_EDITOR
	if (GIsEditor)
	{
		if (LinkedNode == NULL)
		{
			//@TODO: Should only do this when playing back
			AttemptRelink(Context);
		}

		// Record the node line activation
		if (LinkedNode != NULL)
		{
			//@TODO: Might like a cheaper way to check this, like a bool on the Anim Blueprint or the context
			if (UAnimBlueprint* BP = Context.GetAnimBlueprint())
			{
				if( Context.AnimInstance && (BP->GetObjectBeingDebugged() == Context.AnimInstance))
				{
					Context.GetAnimBlueprintClass()->GetAnimBlueprintDebugData().RecordNodeVisit(LinkID, SourceLinkID, Context.GetFinalBlendWeight());
				}
			}
		}
	}
#endif

	if (LinkedNode != NULL)
	{
		LinkedNode->Update(Context);
	}
}

/////////////////////////////////////////////////////
// FPoseLink

void FPoseLink::Evaluate(FPoseContext& Output)
{
	checkf( !bProcessed, TEXT( "Evaluate already in progress, circular link for AnimInstance [%s] Blueprint [%s]" ), \
		Output.AnimInstance ? *Output.AnimInstance->GetFullName() : TEXT( "None" ), Output.GetAnimBlueprintClass() ? *Output.GetAnimBlueprintClass()->GetFullName() : TEXT( "None" ) );
	TGuardValue<bool> CircularGuard(bProcessed, true);

#if WITH_EDITOR
	if ((LinkedNode == NULL) && GIsEditor)
	{
		//@TODO: Should only do this when playing back
		AttemptRelink(Output);
	}
#endif

	if (LinkedNode != NULL)
	{
		LinkedNode->Evaluate(Output);
	}
	else
	{
		//@TODO: Warning here?
		Output.ResetToRefPose();
	}

	// Detect non valid output
	checkSlow( !Output.ContainsNaN() );
	checkSlow( Output.IsNormalized() );
}

/////////////////////////////////////////////////////
// FComponentSpacePoseLink

void FComponentSpacePoseLink::EvaluateComponentSpace(FComponentSpacePoseContext& Output)
{
	checkf( !bProcessed, TEXT( "EvaluateComponentSpace already in progress, circular link for AnimInstance [%s] Blueprint [%s]" ), \
		Output.AnimInstance ? *Output.AnimInstance->GetFullName() : TEXT( "None" ), Output.GetAnimBlueprintClass() ? *Output.GetAnimBlueprintClass()->GetFullName() : TEXT( "None" ) );
	TGuardValue<bool> CircularGuard(bProcessed, true);

	if (LinkedNode != NULL)
	{
		LinkedNode->EvaluateComponentSpace(Output);
	}
	else
	{
		//@TODO: Warning here?
		Output.ResetToRefPose();
	}

	// Detect non valid output
	checkSlow( !Output.ContainsNaN() );
	checkSlow( Output.IsNormalized() );
}

/////////////////////////////////////////////////////
// FPoseContext

bool FPoseContext::ContainsNaN() const
{
	checkSlow( AnimInstance && AnimInstance->RequiredBones.IsValid() );
	const TArray<FBoneIndexType> & RequiredBoneIndices = AnimInstance->RequiredBones.GetBoneIndicesArray();
	for (int32 Iter = 0; Iter < RequiredBoneIndices.Num(); ++Iter)
	{
		const int32 BoneIndex = RequiredBoneIndices[Iter];
		if (Pose.Bones[BoneIndex].ContainsNaN())
		{
			return true;
		}
	}

	return false;
}

bool FPoseContext::IsNormalized() const
{
	checkSlow( AnimInstance && AnimInstance->RequiredBones.IsValid() );
	const TArray<FBoneIndexType> & RequiredBoneIndices = AnimInstance->RequiredBones.GetBoneIndicesArray();
	for (int32 Iter = 0; Iter < RequiredBoneIndices.Num(); ++Iter)
	{
		const int32 BoneIndex = RequiredBoneIndices[Iter];
		if( !Pose.Bones[BoneIndex].IsRotationNormalized() )
		{
			return false;
		}
	}

	return true;
}

/////////////////////////////////////////////////////
// FComponentSpacePoseContext

bool FComponentSpacePoseContext::ContainsNaN() const
{
	checkSlow( AnimInstance && AnimInstance->RequiredBones.IsValid() );
	const TArray<FBoneIndexType> & RequiredBoneIndices = AnimInstance->RequiredBones.GetBoneIndicesArray();
	for (int32 Iter = 0; Iter < RequiredBoneIndices.Num(); ++Iter)
	{
		const int32 BoneIndex = RequiredBoneIndices[Iter];
		if (Pose.Bones[BoneIndex].ContainsNaN())
		{
			return true;
		}
	}

	return false;
}

bool FComponentSpacePoseContext::IsNormalized() const
{
	checkSlow( AnimInstance && AnimInstance->RequiredBones.IsValid() );
	const TArray<FBoneIndexType> & RequiredBoneIndices = AnimInstance->RequiredBones.GetBoneIndicesArray();
	for (int32 Iter = 0; Iter < RequiredBoneIndices.Num(); ++Iter)
	{
		const int32 BoneIndex = RequiredBoneIndices[Iter];
		if( !Pose.Bones[BoneIndex].IsRotationNormalized() )
		{
			return false;
		}
	}

	return true;
}