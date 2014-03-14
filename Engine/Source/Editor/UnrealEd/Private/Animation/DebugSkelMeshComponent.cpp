// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "UnrealEd.h"
#include "SkeletalRenderPublic.h"
#include "AnimationRuntime.h"
#include "AnimPreviewInstance.h"

//////////////////////////////////////////////////////////////////////////
// FDebugSkelMeshSceneProxy

/**
* A skeletal mesh component scene proxy with additional debugging options.
*/
class FDebugSkelMeshSceneProxy : public FSkeletalMeshSceneProxy
{
private:
	/** Holds onto the skeletal mesh component that created it so it can handle the rendering of bones properly. */
	const UDebugSkelMeshComponent* SkeletalMeshComponent;
public:
	/** 
	* Constructor. 
	* @param	Component - skeletal mesh primitive being added
	*/
	FDebugSkelMeshSceneProxy(const UDebugSkelMeshComponent* InComponent, FSkeletalMeshResource* InSkelMeshResource, const FColor& InWireframeOverlayColor = FColor(255, 255, 255)) :
		FSkeletalMeshSceneProxy( InComponent, InSkelMeshResource )
	{
		SkeletalMeshComponent = InComponent;
		WireframeOverlayColor = InWireframeOverlayColor;
	}

	virtual ~FDebugSkelMeshSceneProxy()
	{
	}


	/** 
	* Draw the scene proxy as a dynamic element
	*
	* @param	PDI - draw interface to render to
	* @param	View - current view
	*/
	virtual void DrawDynamicElements(FPrimitiveDrawInterface* PDI,const FSceneView* View)
	{
		// We don't want to draw the mesh geometry to the hit testing render target
		// so that we can get to triangle strips that are partially obscured by other
		// triangle strips easier.
		if (!PDI->IsHitTesting())
		{
			if (SkeletalMeshComponent->bDrawMesh)
			{
				// Draw the mesh
				FSkeletalMeshSceneProxy::DrawDynamicElements(PDI, View);
			}
			
		}

		if( SkeletalMeshComponent->MeshObject && (SkeletalMeshComponent->bDrawNormals || SkeletalMeshComponent->bDrawTangents || SkeletalMeshComponent->bDrawBinormals) )
		{
			SkeletalMeshComponent->MeshObject->DrawVertexElements(PDI, SkeletalMeshComponent->ComponentToWorld, SkeletalMeshComponent->bDrawNormals, SkeletalMeshComponent->bDrawTangents, SkeletalMeshComponent->bDrawBinormals);
		}
	}

	uint32 GetAllocatedSize() const
	{
		return FSkeletalMeshSceneProxy::GetAllocatedSize();
	}

	virtual uint32 GetMemoryFootprint() const OVERRIDE
	{
		return sizeof(*this) + GetAllocatedSize();
	}
};

//////////////////////////////////////////////////////////////////////////
// UDebugSkelMeshComponent

UDebugSkelMeshComponent::UDebugSkelMeshComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bDrawMesh = true;
	PreviewInstance = NULL;
	bDisplayRawAnimation = false;
	bDisplayNonRetargetedPose = false;

	// wind is turned off in the editor by default
	bEnableWind = false;

	bMeshSocketsVisible = true;
	bSkeletonSocketsVisible = true;
}

FBoxSphereBounds UDebugSkelMeshComponent::CalcBounds(const FTransform & LocalToWorld) const
{
	FBoxSphereBounds Result = Super::CalcBounds(LocalToWorld);

	if (! IsUsingInGameBounds())
	{
		// extend bounds by bones but without root bone
		FBox BoundingBox(0);
		for (int32 BoneIndex = 1; BoneIndex < SpaceBases.Num(); ++ BoneIndex)
		{
			BoundingBox += GetBoneMatrix(BoneIndex).GetOrigin();
		}
		Result = Result + FBoxSphereBounds(BoundingBox);
	}

	return Result;
}

bool UDebugSkelMeshComponent::IsUsingInGameBounds() const
{
	return bIsUsingInGameBounds;
}

void UDebugSkelMeshComponent::UseInGameBounds(bool bUseInGameBounds)
{
	bIsUsingInGameBounds = bUseInGameBounds;
}

bool UDebugSkelMeshComponent::CheckIfBoundsAreCorrrect()
{
	if (GetPhysicsAsset())
	{
		bool bWasUsingInGameBounds = IsUsingInGameBounds();
		FTransform TempTransform = FTransform::Identity;
		UseInGameBounds(true);
		FBoxSphereBounds InGameBounds = CalcBounds(TempTransform);
		UseInGameBounds(false);
		FBoxSphereBounds PreviewBounds = CalcBounds(TempTransform);
		UseInGameBounds(bWasUsingInGameBounds);
		// calculate again to have bounds as requested
		CalcBounds(TempTransform);
		// if in-game bounds are of almost same size as preview bounds or bigger, it seems to be fine
		if (! InGameBounds.GetSphere().IsInside(PreviewBounds.GetSphere(), PreviewBounds.GetSphere().W * 0.1f) && // for spheres: A.IsInside(B) checks if A is inside of B
			! PreviewBounds.GetBox().IsInside(InGameBounds.GetBox().ExpandBy(PreviewBounds.GetSphere().W * 0.1f))) // for boxes: A.IsInside(B) checks if B is inside of A
		{
			return true;
		}
	}
	return false;
}

FPrimitiveSceneProxy* UDebugSkelMeshComponent::CreateSceneProxy()
{
	FDebugSkelMeshSceneProxy* Result = NULL;
	ERHIFeatureLevel::Type SceneFeatureLevel = GRHIFeatureLevel;
	FSkeletalMeshResource* SkelMeshResource = SkeletalMesh ? SkeletalMesh->GetResourceForRendering(SceneFeatureLevel) : NULL;

	// only create a scene proxy for rendering if
	// properly initialized
	if( SkelMeshResource && 
		SkelMeshResource->LODModels.IsValidIndex(PredictedLODLevel) &&
		!bHideSkin &&
		MeshObject)
	{
		const FColor WireframeMeshOverlayColor(102,205,170,255);
		Result = ::new FDebugSkelMeshSceneProxy(this, SkelMeshResource, WireframeMeshOverlayColor);
	}

	return Result;
}


bool UDebugSkelMeshComponent::IsPreviewOn() const
{
	return (PreviewInstance != NULL) && (PreviewInstance == AnimScriptInstance);
}

FString UDebugSkelMeshComponent::GetPreviewText() const
{
#define LOCTEXT_NAMESPACE "SkelMeshComponent"

	if (IsPreviewOn())
	{
		if (UBlendSpace* BlendSpace = Cast<UBlendSpace>(PreviewInstance->CurrentAsset))
		{
			return FText::Format( LOCTEXT("BlendSpace", "Blend Space {0}"), FText::FromString(BlendSpace->GetName()) ).ToString();
		}
		else if (UAnimMontage* Montage = Cast<UAnimMontage>(PreviewInstance->CurrentAsset))
		{
			return FText::Format( LOCTEXT("Montage", "Montage {0}"), FText::FromString(Montage->GetName()) ).ToString();
		}
		else if (UAnimSequence* Sequence = Cast<UAnimSequence>(PreviewInstance->CurrentAsset))
		{
			return FText::Format( LOCTEXT("Animation", "Animation {0}"), FText::FromString(Sequence->GetName()) ).ToString();
		}
		else if (UVertexAnimation* VertAnim = Cast<UVertexAnimation>(PreviewInstance->CurrentVertexAnim))
		{
			return FText::Format( LOCTEXT("VertexAnim", "VertexAnim {0}"), FText::FromString(VertAnim->GetName()) ).ToString();
		}
	}

	return LOCTEXT("None", "None").ToString();

#undef LOCTEXT_NAMESPACE
}

void UDebugSkelMeshComponent::InitAnim(bool bForceReinit)
{
	// If we already have PreviewInstnace and its asset's Skeleton does not match with mesh's Skeleton
	// then we need to clear it up to avoid an issue
	if ( PreviewInstance && PreviewInstance->CurrentAsset && SkeletalMesh )
	{
		if ( PreviewInstance->CurrentAsset->GetSkeleton() != SkeletalMesh->Skeleton )
		{
			// if it doesn't match, just clear it
			PreviewInstance->SetAnimationAsset(NULL);
		}
	}

	Super::InitAnim(bForceReinit);

	// if PreviewInstance is NULL, create here once
	if (PreviewInstance == NULL)
	{
		PreviewInstance = ConstructObject<UAnimPreviewInstance>(UAnimPreviewInstance::StaticClass(), this);				
		check(PreviewInstance);

		//Set transactional flag in order to restore slider position when undo operation is performed
		PreviewInstance->SetFlags(RF_Transactional);
	}

	// if anim script instance is null because it's not playing a blueprint, set to PreviewInstnace by default
	// that way if user would like to modify bones or do extra stuff, it will work
	if ((AnimationMode != EAnimationMode::AnimationBlueprint) && (AnimScriptInstance == NULL))
	{
		AnimScriptInstance = PreviewInstance;
		AnimScriptInstance->InitializeAnimation();
	}
}

bool UDebugSkelMeshComponent::IsWindEnabled() const
{
	return bEnableWind;
}


void UDebugSkelMeshComponent::EnablePreview(bool bEnable, UAnimationAsset* PreviewAsset, UVertexAnimation* PreviewVertexAnim)
{
	if (PreviewInstance)
	{
		if (bEnable)
		{
				// back up current AnimInstance if not currently previewing anything
				if (!IsPreviewOn())
				{
					SavedAnimScriptInstance = AnimScriptInstance;
				}

				AnimScriptInstance = PreviewInstance;

				if(PreviewAsset)
				{
					PreviewInstance->SetVertexAnimation(NULL);
					PreviewInstance->SetAnimationAsset(PreviewAsset);
				}
				else
				{
					PreviewInstance->SetAnimationAsset(NULL);
					PreviewInstance->SetVertexAnimation(PreviewVertexAnim);
				}
			}
		else if (IsPreviewOn())
		{
			if (PreviewInstance->CurrentAsset == PreviewAsset || PreviewAsset == NULL)
			{
				// now recover to saved AnimScriptInstance;
				AnimScriptInstance = SavedAnimScriptInstance;
				PreviewInstance->SetAnimationAsset(NULL);
				PreviewInstance->SetVertexAnimation(NULL);
			}
		}
	}
}

bool UDebugSkelMeshComponent::ShouldCPUSkin()
{
	return bDrawBoneInfluences || bDrawNormals || bDrawTangents || bDrawBinormals;
}


void UDebugSkelMeshComponent::PostInitMeshObject(FSkeletalMeshObject* MeshObject)
{
	Super::PostInitMeshObject( MeshObject );

	if (MeshObject)
	{
		if(bDrawBoneInfluences)
		{
			MeshObject->EnableBlendWeightRendering(true, BonesOfInterest);
		}
	}
}

void UDebugSkelMeshComponent::SetShowBoneWeight(bool bNewShowBoneWeight)
{
	// Check we are actually changing it!
	if(bNewShowBoneWeight == bDrawBoneInfluences)
	{
		return;
	}

	// if turning on this mode
	if(bNewShowBoneWeight)
	{
		SkelMaterials.Empty();
		int32 NumMaterials = GetNumMaterials();
		for (int32 i=0; i<NumMaterials; i++)
		{
			// Back up old material
			SkelMaterials.Add(GetMaterial(i));
			// Set special bone weight material
			SetMaterial(i, GEngine->BoneWeightMaterial);
		}
	}
	// if turning it off
	else
	{
		int32 NumMaterials = GetNumMaterials();
		check(NumMaterials == SkelMaterials.Num());
		for (int32 i=0; i<NumMaterials; i++)
		{
			// restore original material
			SetMaterial(i, SkelMaterials[i]);
		}
	}

	bDrawBoneInfluences = bNewShowBoneWeight;
}

void UDebugSkelMeshComponent::RefreshBoneTransforms()
{
	// Run regular update first so we get RequiredBones up to date.
	Super::RefreshBoneTransforms();

	// Non retargeted pose.
	NonRetargetedSpaceBases.Empty();
	if( bDisplayNonRetargetedPose && AnimScriptInstance && AnimScriptInstance->RequiredBones.IsValid() )
	{
		TArray<FTransform> BackupSpaceBases = SpaceBases;

		AnimScriptInstance->RequiredBones.SetDisableRetargeting(true);
		Super::RefreshBoneTransforms();
		AnimScriptInstance->RequiredBones.SetDisableRetargeting(false);

		NonRetargetedSpaceBases = SpaceBases;
		SpaceBases = BackupSpaceBases;
	}

	if( bDisplayRawAnimation )
	{
		// save the transform in CompressedSpaceBases
		CompressedSpaceBases = SpaceBases;

		// use raw data now
		if( AnimScriptInstance && AnimScriptInstance->RequiredBones.IsValid() )
		{
			AnimScriptInstance->RequiredBones.SetUseRAWData(true);
			Super::RefreshBoneTransforms();
			AnimScriptInstance->RequiredBones.SetUseRAWData(false);
		}
		// Otherwise we'll just get ref pose.
		else
		{
			Super::RefreshBoneTransforms();
		}
	}
	else
	{
		CompressedSpaceBases.Empty();
	}

	const bool bIsPreviewInstance = (PreviewInstance && PreviewInstance == AnimScriptInstance);

	// Only works in PreviewInstance, and not for anim blueprint. This is intended.
	AdditiveBasePoses.Empty();
	if( bDisplayAdditiveBasePose && bIsPreviewInstance && PreviewInstance->RequiredBones.IsValid() )
	{
		if (UAnimSequence* Sequence = Cast<UAnimSequence>(PreviewInstance->CurrentAsset)) 
		{ 
			if (Sequence->IsValidAdditive()) 
			{ 
				AdditiveBasePoses.AddUninitialized(PreviewInstance->RequiredBones.GetNumBones());
				Sequence->GetAdditiveBasePose(AdditiveBasePoses, PreviewInstance->RequiredBones, FAnimExtractContext(PreviewInstance->CurrentTime, PreviewInstance->bLooping));
				
				FA2CSPose CSPose;
				CSPose.AllocateLocalPoses(AnimScriptInstance->RequiredBones, AdditiveBasePoses);
				for(int32 i=0; i<AdditiveBasePoses.Num(); ++i)
				{
					AdditiveBasePoses[i] = CSPose.GetComponentSpaceTransform(i);
				}
			}
		}
	}
}
