// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"
#include "AnimGraphNode_TwoBoneIK.h"

// for customization details
#include "../../PropertyEditor/Public/PropertyHandle.h"
#include "../../PropertyEditor/Public/DetailLayoutBuilder.h"
#include "../../PropertyEditor/Public/DetailCategoryBuilder.h"
#include "../../PropertyEditor/Public/DetailWidgetRow.h"
#include "../../PropertyEditor/Public/IDetailPropertyRow.h"

#define LOCTEXT_NAMESPACE "A3Nodes"

/////////////////////////////////////////////////////
// FTwoBoneIKDelegate

class FTwoBoneIKDelegate : public TSharedFromThis<FTwoBoneIKDelegate>
{
public:
	void UpdateLocationSpace(class IDetailLayoutBuilder* DetailBuilder)
	{
		if (DetailBuilder)
		{
			DetailBuilder->ForceRefreshDetails();
		}
	}
};

/////////////////////////////////////////////////////
// UAnimGraphNode_TwoBoneIK


UAnimGraphNode_TwoBoneIK::UAnimGraphNode_TwoBoneIK(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

FText UAnimGraphNode_TwoBoneIK::GetControllerDescription() const
{
	return LOCTEXT("TwoBoneIK", "Two Bone IK");
}

FText UAnimGraphNode_TwoBoneIK::GetTooltipText() const
{
	return LOCTEXT("AnimGraphNode_TwoBoneIK_Tooltip", "The Two Bone IK control applies an inverse kinematic (IK) solver to a 3-joint chain, such as the limbs of a character.");
}

FText UAnimGraphNode_TwoBoneIK::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if ((TitleType == ENodeTitleType::ListView || TitleType == ENodeTitleType::MenuTitle) && (Node.IKBone.BoneName == NAME_None))
	{
		return GetControllerDescription();
	}
	else if (!CachedNodeTitles.IsTitleCached(TitleType))
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("ControllerDescription"), GetControllerDescription());
		Args.Add(TEXT("BoneName"), FText::FromName(Node.IKBone.BoneName));

		// FText::Format() is slow, so we cache this to save on performance
		if (TitleType == ENodeTitleType::ListView || TitleType == ENodeTitleType::MenuTitle)
		{
			CachedNodeTitles.SetCachedTitle(TitleType, FText::Format(LOCTEXT("AnimGraphNode_IKBone_ListTitle", "{ControllerDescription} - Bone: {BoneName}"), Args));
		}
		else
		{
			CachedNodeTitles.SetCachedTitle(TitleType, FText::Format(LOCTEXT("AnimGraphNode_IKBone_Title", "{ControllerDescription}\nBone: {BoneName}"), Args));
		}
	}
	return CachedNodeTitles[TitleType];
}

void UAnimGraphNode_TwoBoneIK::Draw( FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* SkelMeshComp ) const
{
	if (SkelMeshComp && SkelMeshComp->SkeletalMesh && SkelMeshComp->SkeletalMesh->Skeleton)
	{
		USkeleton * Skeleton = SkelMeshComp->SkeletalMesh->Skeleton;

		DrawTargetLocation(PDI, SkelMeshComp, Skeleton, Node.EffectorLocationSpace, Node.EffectorSpaceBoneName, Node.EffectorLocation, FColor(255, 128, 128), FColor(180, 128, 128));
		DrawTargetLocation(PDI, SkelMeshComp, Skeleton, Node.JointTargetLocationSpace, Node.JointTargetSpaceBoneName, Node.JointTargetLocation, FColor(128, 255, 128), FColor(128, 180, 128));
	}
}

void UAnimGraphNode_TwoBoneIK::DrawTargetLocation(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* SkelComp, USkeleton * Skeleton, uint8 SpaceBase, FName SpaceBoneName, const FVector & TargetLocation, const FColor & TargetColor, const FColor & BoneColor) const
{
	const bool bInBoneSpace = (SpaceBase == BCS_ParentBoneSpace) || (SpaceBase == BCS_BoneSpace);
	const int32 SpaceBoneIndex = bInBoneSpace ? Skeleton->GetReferenceSkeleton().FindBoneIndex(SpaceBoneName) : INDEX_NONE;
	// Transform EffectorLocation from EffectorLocationSpace to ComponentSpace.
	FTransform TargetTransform = FTransform (TargetLocation);
	FTransform CSTransform;
	ConvertToComponentSpaceTransform(SkelComp, Skeleton, TargetTransform, CSTransform, SpaceBoneIndex, SpaceBase);

	FTransform WorldTransform = CSTransform * SkelComp->ComponentToWorld;

	DrawCoordinateSystem( PDI, WorldTransform.GetLocation(), WorldTransform.GetRotation().Rotator(), 20.f, SDPG_Foreground );
	DrawWireDiamond( PDI, WorldTransform.ToMatrixWithScale(), 2.f, TargetColor, SDPG_Foreground );

	if (bInBoneSpace)
	{
		ConvertToComponentSpaceTransform(SkelComp, Skeleton, FTransform::Identity, CSTransform, SpaceBoneIndex, SpaceBase);
		WorldTransform = CSTransform * SkelComp->ComponentToWorld;
		DrawCoordinateSystem( PDI, WorldTransform.GetLocation(), WorldTransform.GetRotation().Rotator(), 20.f, SDPG_Foreground );
		DrawWireDiamond( PDI, WorldTransform.ToMatrixWithScale(), 2.f, BoneColor, SDPG_Foreground );
	}
}

void UAnimGraphNode_TwoBoneIK::ConvertToComponentSpaceTransform(USkeletalMeshComponent* SkelComp, USkeleton * Skeleton, const FTransform & InTransform, FTransform & OutCSTransform, int32 BoneIndex, uint8 Space) const
{
	switch( Space )
	{
	case BCS_WorldSpace : 
		{
			OutCSTransform = InTransform;
			OutCSTransform.SetToRelativeTransform(SkelComp->ComponentToWorld);
		}
		break;

	case BCS_ComponentSpace :
		{
			// Component Space, no change.
			OutCSTransform = InTransform;
		}
		break;

	case BCS_ParentBoneSpace :
		if( BoneIndex != INDEX_NONE )
		{
			const int32 ParentIndex = Skeleton->GetReferenceSkeleton().GetParentIndex(BoneIndex);
			if( ParentIndex != INDEX_NONE )
			{
				const int32 MeshParentIndex = Skeleton->GetMeshBoneIndexFromSkeletonBoneIndex(SkelComp->SkeletalMesh, ParentIndex);
				if (MeshParentIndex != INDEX_NONE)
				{
					const FTransform ParentTM = SkelComp->GetBoneTransform(MeshParentIndex);
					OutCSTransform = InTransform * ParentTM;
				}
				else
				{
					OutCSTransform = InTransform;
				}
			}
		}
		break;

	case BCS_BoneSpace :
		if( BoneIndex != INDEX_NONE )
		{
			const int32 MeshBoneIndex = Skeleton->GetMeshBoneIndexFromSkeletonBoneIndex(SkelComp->SkeletalMesh, BoneIndex);
			if (MeshBoneIndex != INDEX_NONE)
			{
				const FTransform BoneTM = SkelComp->GetBoneTransform(MeshBoneIndex);
				OutCSTransform = InTransform * BoneTM;
			}
			else
			{
				OutCSTransform = InTransform;
			}
		}
		break;

	default:
		if( SkelComp->SkeletalMesh )
		{
			UE_LOG(LogAnimation, Warning, TEXT("ConvertToComponentSpaceTransform: Unknown BoneSpace %d  for Mesh: %s"), Space, *SkelComp->SkeletalMesh->GetFName().ToString() );
		}
		else
		{
			UE_LOG(LogAnimation, Warning, TEXT("ConvertToComponentSpaceTransform: Unknown BoneSpace %d  for Skeleton: %s"), Space, *Skeleton->GetFName().ToString() );
		}
		break;
	}
}

// just for refreshing UIs when bone space was changed
TSharedPtr<FTwoBoneIKDelegate> TwoBoneIKDelegate;

void UAnimGraphNode_TwoBoneIK::CustomizeDetails(class IDetailLayoutBuilder& DetailBuilder)
{
	// initialize just once
	if (!TwoBoneIKDelegate.IsValid())
	{
		TwoBoneIKDelegate = MakeShareable(new FTwoBoneIKDelegate());
	}

	EBoneControlSpace Space = Node.EffectorLocationSpace;
	if (Space == BCS_BoneSpace || Space == BCS_ParentBoneSpace)
	{
		IDetailCategoryBuilder& IKCategory = DetailBuilder.EditCategory("IK");
		IDetailCategoryBuilder& EffectorCategory = DetailBuilder.EditCategory("EndEffector");
		TSharedPtr<IPropertyHandle> PropertyHandle;
		PropertyHandle = DetailBuilder.GetProperty(FName("Node.bTakeRotationFromEffectorSpace"), GetClass());
		EffectorCategory.AddProperty(PropertyHandle);
		PropertyHandle = DetailBuilder.GetProperty(FName("Node.bMaintainEffectorRelRot"), GetClass());
		EffectorCategory.AddProperty(PropertyHandle);
		PropertyHandle = DetailBuilder.GetProperty(FName("Node.EffectorSpaceBoneName"), GetClass());
		EffectorCategory.AddProperty(PropertyHandle);
	}
	else // hide all properties in EndEffector category
	{
		TSharedPtr<IPropertyHandle> PropertyHandle = DetailBuilder.GetProperty(FName("Node.EffectorLocation"), GetClass());
		DetailBuilder.HideProperty(PropertyHandle);
		PropertyHandle = DetailBuilder.GetProperty(FName("Node.bTakeRotationFromEffectorSpace"), GetClass());
		DetailBuilder.HideProperty(PropertyHandle);
		PropertyHandle = DetailBuilder.GetProperty(FName("Node.bMaintainEffectorRelRot"), GetClass());
		DetailBuilder.HideProperty(PropertyHandle);
		PropertyHandle = DetailBuilder.GetProperty(FName("Node.EffectorSpaceBoneName"), GetClass());
		DetailBuilder.HideProperty(PropertyHandle);
	}

	Space = Node.JointTargetLocationSpace;
	if (Space == BCS_BoneSpace || Space == BCS_ParentBoneSpace)
	{
		IDetailCategoryBuilder& IKCategory = DetailBuilder.EditCategory("IK");
		IDetailCategoryBuilder& EffectorCategory = DetailBuilder.EditCategory("JointTarget");
		TSharedPtr<IPropertyHandle> PropertyHandle;
		PropertyHandle = DetailBuilder.GetProperty(FName("Node.JointTargetSpaceBoneName"), GetClass());
		EffectorCategory.AddProperty(PropertyHandle);

		SetPinsVisibility(true);
	}
	else // hide all properties in JointTarget category except for JointTargetLocationSpace
	{
		TSharedPtr<IPropertyHandle> PropertyHandle = DetailBuilder.GetProperty(FName("Node.JointTargetLocation"), GetClass());
		DetailBuilder.HideProperty(PropertyHandle);
		PropertyHandle = DetailBuilder.GetProperty(FName("Node.JointTargetSpaceBoneName"), GetClass());
		DetailBuilder.HideProperty(PropertyHandle);

		SetPinsVisibility(false);
	}


	// refresh UIs when bone space is changed
	TSharedRef<IPropertyHandle> EffectorLocHandle = DetailBuilder.GetProperty(FName("Node.EffectorLocationSpace"), GetClass());
	if (EffectorLocHandle->IsValidHandle())
	{
		FSimpleDelegate UpdateEffectorSpaceDelegate = FSimpleDelegate::CreateSP(TwoBoneIKDelegate.Get(), &FTwoBoneIKDelegate::UpdateLocationSpace, &DetailBuilder);
		EffectorLocHandle->SetOnPropertyValueChanged(UpdateEffectorSpaceDelegate);
	}

	TSharedRef<IPropertyHandle> JointTragetLocHandle = DetailBuilder.GetProperty(FName("Node.JointTargetLocationSpace"), GetClass());
	if (JointTragetLocHandle->IsValidHandle())
	{
		FSimpleDelegate UpdateJointSpaceDelegate = FSimpleDelegate::CreateSP(TwoBoneIKDelegate.Get(), &FTwoBoneIKDelegate::UpdateLocationSpace, &DetailBuilder);
		JointTragetLocHandle->SetOnPropertyValueChanged(UpdateJointSpaceDelegate);
	}

	// reconstruct node for showing/hiding Pins
	ReconstructNode();
}

void UAnimGraphNode_TwoBoneIK::SetPinsVisibility(bool bShow)
{
	for (FOptionalPinFromProperty& Pin : ShowPinForProperties)
	{
		if (Pin.PropertyName == "JointTargetLocation")
		{
			PreEditChange(NULL);
			Pin.bShowPin = bShow;
			break;
		}
	}
}

#undef LOCTEXT_NAMESPACE
