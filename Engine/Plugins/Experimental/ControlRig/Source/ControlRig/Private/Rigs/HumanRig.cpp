// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "HumanRig.h"
#include "TwoBoneIK.h"

// initializing and caching didn't work well in particular when you keep recompiling BP, so I'm just getting it all the time
/////////////////////////////////////////////////////
// Custom Property
template< typename T, typename P >
T FCustomProperty::GetValue() const
{
	if (ensure(IsValid()))
	{
		UProperty* CachedProperty = Container->GetClass()->FindPropertyByName(PropertyToCache);
		if (CachedProperty->IsA<P>())
		{
			return *CachedProperty->ContainerPtrToValuePtr<T>(Container, 0);
		}
	}

	// @fixme: return is ambiguous - this will return just empty for everything
	// this won't compile if you have different property, so we'll have to fix that
	return (T)0;
}

template< typename T, typename P >
void FCustomProperty::SetValue(const T& Value)
{
	if (ensure(IsValid()))
	{
		UProperty* CachedProperty = Container->GetClass()->FindPropertyByName(PropertyToCache);
		if (CachedProperty->IsA<P>())
		{
			NotifyPreEditChangeProperty(Container, CachedProperty);
			T& PropertyValue = *CachedProperty->ContainerPtrToValuePtr<T>(Container, 0);
			PropertyValue = Value;
			NotifyPostEditChangeProperty(Container, CachedProperty);
		}
	}
}

void FCustomProperty::NotifyPreEditChangeProperty(UObject* InContainer, UProperty* CachedProperty)
{
#if WITH_EDITOR
	if (CachedProperty && InContainer)
	{
		FEditPropertyChain EditPropertyChain;
		EditPropertyChain.AddHead(CachedProperty);
		EditPropertyChain.SetActivePropertyNode(CachedProperty);
		InContainer->PreEditChange(EditPropertyChain);
	}
#endif
}

void FCustomProperty::NotifyPostEditChangeProperty(UObject* InContainer, UProperty* CachedProperty)
{
#if WITH_EDITOR
	if (CachedProperty && InContainer)
	{
		FPropertyChangedEvent PropertyChangedEvent(CachedProperty);
		InContainer->PostEditChangeProperty(PropertyChangedEvent);
	}
#endif
}
/////////////////////////////////////////////////////
// UHumanRig

UHumanRig::UHumanRig(const FObjectInitializer& ObjectInitializer)
{

}

void UHumanRig::Evaluate()
{
	Super::Evaluate();

	// update control node position
	for (int32 LimbControlIndex = 0; LimbControlIndex < Limbs.Num(); ++LimbControlIndex)
	{
		FLimbControl& LimbControl = Limbs[LimbControlIndex];

		// make sure we're in the correct space first
		CorrectIKSpace(LimbControl);

		// IK solver
		FTransform RootTransform = GetGlobalTransform(LimbControl.IKChainName[0]);
		FTransform JointTransform = GetGlobalTransform(LimbControl.IKChainName[1]);
		FTransform EndTransform = GetGlobalTransform(LimbControl.IKChainName[2]);
		FTransform InitEndTransform = EndTransform;

		FVector JointTargetPos, DesiredPos;
		JointTargetPos = GetGlobalTransform(LimbControl.IKJointTargetName).GetLocation();
		DesiredPos = GetGlobalTransform(LimbControl.IKEffectorName).GetLocation();

		AnimationCore::SolveTwoBoneIK(RootTransform, JointTransform,  EndTransform, JointTargetPos, DesiredPos, LimbControl.UpperLimbLength, LimbControl.LowerLimbLength, false, 1.f, 1.f);

		// only want to do this for rotation, but at the end, I have to copy the rotation from FK and apply the delta
		//EndTransform = InitEndTransform * JointTransform;

		SetGlobalTransform(LimbControl.IKChainName[0], RootTransform);
		SetGlobalTransform(LimbControl.IKChainName[1], JointTransform);
		SetGlobalTransform(LimbControl.IKChainName[2], EndTransform);

		// now blend between by weight
		float BlendWeight = LimbControl.IKBlendWeight.GetValue<float, UFloatProperty>();
		SetGlobalTransform(LimbControl.ResultChain[0], Lerp(LimbControl.IKChainName[0], LimbControl.FKChainName[0], BlendWeight));
		SetGlobalTransform(LimbControl.ResultChain[1], Lerp(LimbControl.IKChainName[1], LimbControl.FKChainName[1], BlendWeight));
		SetGlobalTransform(LimbControl.ResultChain[2], Lerp(LimbControl.IKChainName[2], LimbControl.FKChainName[2], BlendWeight));
	}
}

void UHumanRig::Initialize()
{
	Super::Initialize();

	// update control node position
	for (int32 LimbControlIndex = 0; LimbControlIndex < Limbs.Num(); ++LimbControlIndex)
	{
		const FLimbControl& LimbControl = Limbs[LimbControlIndex];
		FTransform Upper = GetGlobalTransform(LimbControl.IKChainName[0]);
		FTransform Middle = GetGlobalTransform(LimbControl.IKChainName[1]);
		FTransform Lower = GetGlobalTransform(LimbControl.IKChainName[2]);
		float UpperLimbLength = (Upper.GetLocation() - Middle.GetLocation()).Size();
		float LowerLimbLength = (Middle.GetLocation() - Lower.GetLocation()).Size();
		Limbs[LimbControlIndex].Initialize(this, UpperLimbLength, LowerLimbLength);
	}
}

#if WITH_EDITOR
void UHumanRig::AddLeg(FName UpperLegNode, FName LowerLegNode, FName AnkleLegNode)
{
	FLimbControl LimbControl;

	FTransform UpperLegNodeTransform = GetGlobalTransform(UpperLegNode);
	FTransform LowerLegNodeTransform = GetGlobalTransform(LowerLegNode);
	FTransform AnkleLegNodeTransform = GetGlobalTransform(AnkleLegNode);

	LimbControl.ResultChain[0] = UpperLegNode;
	LimbControl.ResultChain[1] = LowerLegNode;
	LimbControl.ResultChain[2] = AnkleLegNode;

	FName NewNodeName = FName(*FString(UpperLegNode.ToString() + TEXT("_FK")));
	AddFKNode(NewNodeName, LimbControl.FKChainName[0], NAME_None, UpperLegNodeTransform, UpperLegNode);

	NewNodeName = FName(*FString(LowerLegNode.ToString() + TEXT("_FK")));
	AddFKNode(NewNodeName, LimbControl.FKChainName[1], LimbControl.FKChainName[0], LowerLegNodeTransform, LowerLegNode);

	NewNodeName = FName(*FString(AnkleLegNode.ToString() + TEXT("_FK")));
	AddFKNode(NewNodeName, LimbControl.FKChainName[2], LimbControl.FKChainName[1], AnkleLegNodeTransform, AnkleLegNode);

	// Add IK nodes
	NewNodeName = FName(*FString(UpperLegNode.ToString() + TEXT("_IK")));
	EnsureUniqueName(NewNodeName);
	AddNode(NewNodeName, NAME_None, UpperLegNodeTransform, UpperLegNode);
	LimbControl.IKChainName[0] = NewNodeName;

	FName ParentNodeName = NewNodeName;
	NewNodeName = FName(*FString(LowerLegNode.ToString() + TEXT("_IK")));
	EnsureUniqueName(NewNodeName);
	AddNode(NewNodeName, ParentNodeName, LowerLegNodeTransform, LowerLegNode);
	LimbControl.IKChainName[1] = NewNodeName;

	ParentNodeName = NewNodeName;
	NewNodeName = FName(*FString(AnkleLegNode.ToString() + TEXT("_IK")));
	EnsureUniqueName(NewNodeName);
	AddNode(NewNodeName, ParentNodeName, AnkleLegNodeTransform, AnkleLegNode);
	LimbControl.IKChainName[2] = NewNodeName;

	// add IK chains
	AddTwoBoneIK(UpperLegNode, LowerLegNode, AnkleLegNode, LimbControl.IKJointTargetName, LimbControl.IKEffectorName);
	Limbs.Add(LimbControl);

	Sort();
}

void UHumanRig::EnsureUniqueName(FName& InOutNodeName)
{
	FName NewNodeName = InOutNodeName;
	int32 SuffixIndex = 1;
	const FAnimationHierarchy& MyHierarchy = GetHierarchy();
	while (MyHierarchy.Contains(NewNodeName))
	{
		TArray<FStringFormatArg> Args;
		Args.Add(InOutNodeName.ToString());
		Args.Add(FString::FormatAsNumber(SuffixIndex));
		NewNodeName = FName(*FString::Format(TEXT("{0}_{1}"), Args));
		++SuffixIndex;
	}

	InOutNodeName = NewNodeName;
}

void UHumanRig::AddFKNode(FName& InOutNodeName, FName& OutCtrlNodeName, FName InParentNode, FTransform InTransform, FName LinkNode)
{
	// should verify name
	EnsureUniqueName(InOutNodeName);
	AddNode(InOutNodeName, InParentNode, InTransform, LinkNode);

	// add control name
	OutCtrlNodeName = FName(*FString(InOutNodeName.ToString() + TEXT("_Ctrl")));
	EnsureUniqueName(OutCtrlNodeName);
	AddNode(OutCtrlNodeName, InOutNodeName, InTransform, LinkNode);
}

void UHumanRig::AddTwoBoneIK(FName UpperNode, FName MiddleNode, FName EndNode, FName& OutJointTarget, FName& OutEffector)
{
	FTransform JointTarget = GetGlobalTransform(MiddleNode);
	JointTarget.SetRotation(FQuat::Identity);
	// this doens't work anymore because it's using property for manipulator
	OutJointTarget = FName(*FString(EndNode.ToString() + TEXT("_IK_JointTarget")));
	EnsureUniqueName(OutJointTarget);
	AddNode(OutJointTarget, NAME_None, JointTarget);

	FTransform EndEffector = GetGlobalTransform(EndNode);
	EndEffector.SetRotation(FQuat::Identity);
	// this doens't work anymore because it's using property for manipulator
	OutEffector = FName(*FString(EndNode.ToString() + TEXT("_IK_Effector")));
	EnsureUniqueName(OutEffector);
	AddNode(OutEffector, NAME_None, EndEffector);
}
#endif // WITH_EDITOR

FTransform UHumanRig::Lerp(const FName& ANode, const FName& BNode, float Weight)
{
	FTransform ATransform = GetGlobalTransform(ANode);
	FTransform BTransform = GetGlobalTransform(BNode);
	BTransform.BlendWith(ATransform, Weight);

	return BTransform;
}

void UHumanRig::SwitchToIK(FLimbControl& Control)
{
	// switch to IK mode
	//we only focus on 
	FTransform EndTransform = GetGlobalTransform(Control.ResultChain[2]);
	FTransform MidTransform = GetGlobalTransform(Control.ResultChain[1]);
	FTransform RootTransform = GetGlobalTransform(Control.ResultChain[0]);

	// now get the plain of 3 points
	// get joint target
	// first get normal dir to mid joint 
	FVector BaseVector = (EndTransform.GetLocation() - RootTransform.GetLocation()).GetSafeNormal();
	FVector DirToMid = (MidTransform.GetLocation() - RootTransform.GetLocation()).GetSafeNormal();

	// if it's not parallel
	if (FMath::Abs(FVector::DotProduct(BaseVector, DirToMid)) < 0.999f)
	{
		FVector UpVector = FVector::CrossProduct(BaseVector, DirToMid).GetSafeNormal();
		FVector NewDir = FVector::CrossProduct(BaseVector, UpVector);

		// make sure new dir is aligning with dirtomid
		if (FVector::DotProduct(NewDir, DirToMid) < 0.f)
		{
			NewDir *= -1;
		}

		FTransform JointTransform = MidTransform;
		JointTransform.SetLocation(MidTransform.GetLocation() + NewDir * 100.f);

		SetGlobalTransform(Control.IKJointTargetName, JointTransform);
	}
	else
	{
		// @todo fixme:
		SetGlobalTransform(Control.IKJointTargetName, MidTransform);
	}
	// effector transform is simple. See if rotation works
	FTransform EndEffector = EndTransform;
	//EndEffector.SetLocation(EndTransform.GetLocation());
	SetGlobalTransform(Control.IKEffectorName, EndEffector);

	for (int32 Index = 0; Index < 3; ++Index)
	{
		FTransform FKTransform = GetGlobalTransform(Control.ResultChain[Index]);
		SetGlobalTransform(Control.IKChainName[Index], FKTransform);
	}
}

void UHumanRig::SwitchToFK(FLimbControl& Control)
{
	// switch to FK mode
	// copy all result node transform to FK
	// I think I only have to copy result
	for (int32 Index = 0; Index < 3; ++Index)
	{
		FTransform FKTransform = GetGlobalTransform(Control.ResultChain[Index]);
		SetGlobalTransform(Control.FKChainName[Index], FKTransform);
	}
}

void UHumanRig::CorrectIKSpace(FLimbControl& Control)
{
	// if use weight, don't touch it
	EIKSpaceMode CurrentSpaceMode = Control.IKSpaceMode.GetValue<EIKSpaceMode, UEnumProperty>();
	if (CurrentSpaceMode != EIKSpaceMode::UseWeight)
	{
		float BlendWeight = Control.IKBlendWeight.GetValue<float, UFloatProperty>();

		if (CurrentSpaceMode == EIKSpaceMode::SwitchToFK)
		{
			// if 0.f it is already in FK mode
			if (BlendWeight != 0.f)
			{
				SwitchToFK(Control);

				Control.IKBlendWeight.SetValue<float, UFloatProperty>(0.f);
			}

			// I can't change to useweight this value has to be keyable, you have to change to UseWeight if you want to animate again
		}
		else  if (CurrentSpaceMode == EIKSpaceMode::SwitchToIK)
		{
			// if 1.f, it is already in IK mode
			if (BlendWeight != 1.f)
			{
				SwitchToIK(Control);
				Control.IKBlendWeight.SetValue<float, UFloatProperty>(1.f);
			}

			// I can't change to useweight this value has to be keyable, you have to change to UseWeight if you want to animate again
		}
	}
}

bool UHumanRig::GetIKSpaceForNode(FName Node, EIKSpaceMode& OutIKSpace) const
{
	for (const FLimbControl& LimbControl : Limbs)
	{
		if (LimbControl.IKEffectorName == Node || LimbControl.IKJointTargetName == Node)
		{
			OutIKSpace = LimbControl.IKSpaceMode.GetValue<EIKSpaceMode, UEnumProperty>();
			return true;
		}

		for (int32 Index = 0; Index < 3; ++Index)
		{
			if (LimbControl.FKChainName[Index] == Node)
			{
				OutIKSpace = LimbControl.IKSpaceMode.GetValue<EIKSpaceMode, UEnumProperty>();
				return true;
			}
			if (LimbControl.IKChainName[Index] == Node)
			{
				OutIKSpace = LimbControl.IKSpaceMode.GetValue<EIKSpaceMode, UEnumProperty>();
				return true;
			}
		}
	}

	return false;
}

bool UHumanRig::IsManipulatorEnabled(UControlManipulator* InManipulator) const
{
	EIKSpaceMode IKSpaceForManipulator;
	if (GetIKSpaceForNode(InManipulator->Name, IKSpaceForManipulator))
	{
		return IKSpaceForManipulator == EIKSpaceMode::UseWeight || IKSpaceForManipulator == InManipulator->KinematicSpace;
	}
	
	return true;
}

///////////////////////////////////////////////////////////////////
//
// FLimbControl
/////////////////////////////////////////////////////////////
void FLimbControl::Initialize(UObject* InContainer, float InUpperLimbLen, float InLowerLimbLen)
{
	check(InContainer);

	IKBlendWeight.Initialize(InContainer);
	IKSpaceMode.Initialize(InContainer);

	UpperLimbLength = InUpperLimbLen;
	LowerLimbLength = InLowerLimbLen;
}