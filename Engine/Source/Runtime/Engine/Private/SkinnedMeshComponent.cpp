// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	UnSkeletalComponent.cpp: Actor component implementation.
=============================================================================*/

#include "EnginePrivate.h"
#include "ParticleDefinitions.h"
#include "BlueprintUtilities.h"
#include "SkeletalRenderCPUSkin.h"
#include "SkeletalRenderGPUSkin.h"
#include "EngineInterpolationClasses.h"
#include "AnimationUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogSkinnedMeshComp, Log, All);

int32 GSkeletalMeshLODBias = 0;
FAutoConsoleVariableRef CVarSkeletalMeshLODBias(
	TEXT("r.SkeletalMeshLODBias"),
	GSkeletalMeshLODBias,
	TEXT("LOD bias for skeletal meshes."),
	ECVF_Scalability
	);

void USkinnedMeshComponent::OnRegister()
{
	// The reason this happens before register
	// is so that any transform update (or children transform update)
	// won't result in any issues of accessing SpaceBases
	// This isn't really ideal solution because these transform won't have 
	// any valid data yet. 
	AllocateTransformData();
	UpdateMasterBoneMap();	

	Super::OnRegister();

	UpdateLODStatus();
}

void USkinnedMeshComponent::OnUnregister()
{
	DeallocateTransformData();
	Super::OnUnregister();
}

void USkinnedMeshComponent::CreateRenderState_Concurrent()
{
	if( SkeletalMesh )
	{
		// Initialize the alternate weight tracks if present BEFORE creating the new mesh object
		InitLODInfos();

		// No need to create the mesh object if we aren't actually rendering anything (see UPrimitiveComponent::Attach)
		if ( FApp::CanEverRender() && ShouldComponentAddToScene() )
		{
			ERHIFeatureLevel::Type SceneFeatureLevel = GRHIFeatureLevel;
			FSkeletalMeshResource* SkelMeshResource = SkeletalMesh->GetResourceForRendering(SceneFeatureLevel);

			// Also check if skeletal mesh has too many bones/chunk for GPU skinning.
			const bool bIsCPUSkinned = SkelMeshResource->RequiresCPUSkinning(SceneFeatureLevel) || (GIsEditor && ShouldCPUSkin());
			if(bIsCPUSkinned)
			{
				MeshObject = ::new FSkeletalMeshObjectCPUSkin(this,SkelMeshResource);
			}
			else
			{
				MeshObject = ::new FSkeletalMeshObjectGPUSkin(this,SkelMeshResource);
			}

			//Allow the editor a chance to manipulate it before its added to the scene
			PostInitMeshObject(MeshObject);
		}
	}

	Super::CreateRenderState_Concurrent();

	if (SkeletalMesh)
	{
		// Update dynamic data

		if(MeshObject)
		{
			int32 UseLOD = PredictedLODLevel;
			if(MasterPoseComponent)
			{
				UseLOD = FMath::Clamp(MasterPoseComponent->PredictedLODLevel, 0, MeshObject->GetSkeletalMeshResource().LODModels.Num()-1);
			}

			// We just recreated RenderState, that means it could be new mesh or different mesh
			// Clear ActiveVertexAnims, so that it doesn't use from previous state
			ActiveVertexAnims.Empty();
			MeshObject->Update(UseLOD,this,ActiveVertexAnims);  // send to rendering thread
		}

		// scene proxy update of material usage based on active morphs
		UpdateMorphMaterialUsageOnProxy();
	}
}

void USkinnedMeshComponent::DestroyRenderState_Concurrent()
{
	Super::DestroyRenderState_Concurrent();

	if(MeshObject)
	{
		// Begin releasing the RHI resources used by this skeletal mesh component.
		// This doesn't immediately destroy anything, since the rendering thread may still be using the resources.
		MeshObject->ReleaseResources();

		// Begin a deferred delete of MeshObject.  BeginCleanup will call MeshObject->FinishDestroy after the above release resource
		// commands execute in the rendering thread.
		BeginCleanup(MeshObject);
		MeshObject = NULL;
	}
}

FString USkinnedMeshComponent::GetDetailedInfoInternal() const
{
	FString Result;  

	if( SkeletalMesh != NULL )
	{
		Result = SkeletalMesh->GetDetailedInfoInternal();
	}
	else
	{
		Result = TEXT("No_SkeletalMesh");
	}

	return Result;  
}

void USkinnedMeshComponent::SendRenderDynamicData_Concurrent()
{
	SCOPE_CYCLE_COUNTER(STAT_SkelCompUpdateTransform);

	Super::SendRenderDynamicData_Concurrent();

	// if we have not updated the transforms then no need to send them to the rendering thread
	// @todo GIsEditor used to be bUpdateSkelWhenNotRendered. Look into it further to find out why it doesn't update animations in the AnimSetViewer, when a level is loaded in UED (like POC_Cover.gear).
	if( MeshObject && SkeletalMesh && (bForceMeshObjectUpdate || (bRecentlyRendered || MeshComponentUpdateFlag == EMeshComponentUpdateFlag::AlwaysTickPoseAndRefreshBones || GIsEditor || MeshObject->bHasBeenUpdatedAtLeastOnce == false)) )
	{
		SCOPE_CYCLE_COUNTER(STAT_MeshObjectUpdate);

		int32 UseLOD = PredictedLODLevel;
		// If we have a MasterPoseComponent - force this component to render at that LOD, so all bones are present for it.
		// Note that this currently relies on the behaviour where this mesh is rendered at the LOD we pass in here for all viewports. We should
		// be able to render it at lower LOD on viewports where it is further away. That will make the MasterPoseComponent case a bit harder to solve.
		if(MasterPoseComponent)
		{
			UseLOD = FMath::Clamp(MasterPoseComponent->PredictedLODLevel, 0, MeshObject->GetSkeletalMeshResource().LODModels.Num()-1);
		}

		// Are morph targets disabled for this LOD?
		if ( SkeletalMesh->LODInfo[ UseLOD ].bHasBeenSimplified )
		{
			ActiveVertexAnims.Empty();
		}

		if( BoneVisibilityStates.Num() == SpaceBases.Num() )
		{
			// for invisible bones, I'll still need to update the transform, so that rendering can use it for skinning
			uint8 const * BoneVisibilityState = BoneVisibilityStates.GetData();
			FTransform * SpaceBase = SpaceBases.GetData();
			for (int32 BoneIndex=0; BoneIndex<BoneVisibilityStates.Num(); ++BoneIndex, ++BoneVisibilityState, ++SpaceBase)
			{
				if (*BoneVisibilityState!=BVS_Visible)
				{
					if (BoneIndex != 0 )
					{
						// since they're invisible, copy parent transform to itself and scale set to be 0
						int32 const ParentIndex =  SkeletalMesh->RefSkeleton.GetParentIndex(BoneIndex);
						*SpaceBase = SpaceBases[ParentIndex];
					}
					SpaceBase->SetScale3D(FVector::ZeroVector);
				}
			}
		}

		MeshObject->Update(UseLOD,this,ActiveVertexAnims);  // send to rendering thread
		MeshObject->bHasBeenUpdatedAtLeastOnce = true;
		
		// scene proxy update of material usage based on active morphs
		UpdateMorphMaterialUsageOnProxy();
	}

}

#if WITH_EDITOR
void USkinnedMeshComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	UProperty* PropertyThatChanged = PropertyChangedEvent.Property;

	if ( PropertyChangedEvent.Property != NULL )
	{
		if ( GIsEditor && PropertyThatChanged->GetFName() == GET_MEMBER_NAME_CHECKED(USkinnedMeshComponent, StreamingDistanceMultiplier) )
		{
			// Recalculate in a few seconds.
			GEngine->TriggerStreamingDataRebuild();
		}
	}
}
#endif // WITH_EDITOR

void USkinnedMeshComponent::InitLODInfos()
{
	if (SkeletalMesh != NULL)
	{
		if (SkeletalMesh->LODInfo.Num() != LODInfo.Num())
		{
			LODInfo.Empty(SkeletalMesh->LODInfo.Num());
			for (int32 Idx=0; Idx < SkeletalMesh->LODInfo.Num(); Idx++)
			{
				new(LODInfo) FSkelMeshComponentLODInfo();
			}
		}
	}	
}


bool USkinnedMeshComponent::ShouldTickPose() const
{
	return ((MeshComponentUpdateFlag < EMeshComponentUpdateFlag::OnlyTickPoseWhenRendered) || bRecentlyRendered);
}

bool USkinnedMeshComponent::ShouldUpdateTransform(bool bLODHasChanged) const
{
	return (bLODHasChanged || bRecentlyRendered || (MeshComponentUpdateFlag == EMeshComponentUpdateFlag::AlwaysTickPoseAndRefreshBones));
}


void USkinnedMeshComponent::TickUpdateRate()
{
	SCOPE_CYCLE_COUNTER(STAT_TickUpdateRate);
	if( bEnableUpdateRateOptimizations )
	{
		AActor * Owner = GetOwner();
		if( Owner )
		{
			// Convert current frame counter from 64 to 32 bits.
			const uint32 CurrentFrame32 = uint32(GFrameCounter % MAX_uint32);

			// Tick Owner once per frame. All attached SkinnedMeshComponents will share the same settings.
			if( Owner->AnimUpdateRateFrameCount != CurrentFrame32 )
			{
				Owner->AnimUpdateRateFrameCount = CurrentFrame32;
				Owner->AnimUpdateRateTick();
			}

			// debug -- @todo: hook this up to a console command.
			if( bDisplayDebugUpdateRateOptimizations )
			{
				FColor DrawColor;
				switch( Owner->AnimUpdateRateParams.GetUpdateRate() )
				{
				case 1 : DrawColor = FColor::Red; break;
				case 2 : DrawColor = FColor::Green; break;
				case 3 : DrawColor = FColor::Blue; break;
				default: DrawColor = FColor::Black; break;
				}
				DrawDebugBox(GetWorld(), Bounds.Origin, Bounds.BoxExtent, FQuat::Identity, DrawColor, false);
			}
		}
	}
}

void USkinnedMeshComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	SCOPE_CYCLE_COUNTER(STAT_SkinnedMeshCompTick);

	// Tick ActorComponent first.
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// See if this mesh was rendered recently. This has to happen first because other data will rely on this
	bRecentlyRendered = (LastRenderTime > GetWorld()->TimeSeconds - 1.0f);
	TickUpdateRate();

	AActor * Owner = GetOwner();
	const FAnimUpdateRateParameters  & UpdateRateParams = Owner ? Owner->AnimUpdateRateParams : FAnimUpdateRateParameters();

	// Tick Pose first
	if( ShouldTickPose() )
	{
		if( bEnableUpdateRateOptimizations && UpdateRateParams.ShouldSkipUpdate() )
		{
			SkippedTickDeltaTime += DeltaTime;
			if( !bRecentlyRendered )
			{
				return;
			}
		}
		else
		{
			TickPose(DeltaTime + SkippedTickDeltaTime);
			SkippedTickDeltaTime = 0.f;
		}
	}
	else
	{
		SkippedTickDeltaTime = 0.f;
	}

	// Update component's LOD settings
	const bool bLODHasChanged = UpdateLODStatus();

	// If we have been recently rendered, and bForceRefPose has been on for at least a frame, or the LOD changed, update bone matrices.
	if( ShouldUpdateTransform(bLODHasChanged) )
	{
		// Do not update bones if we are taking bone transforms from another SkelMeshComp
		if( MasterPoseComponent )
		{
			UpdateSlaveComponent();
		}
		else 
		{
			RefreshBoneTransforms(); 
		}
	}
}

UObject const* USkinnedMeshComponent::AdditionalStatObject() const
{
	return SkeletalMesh;
}


void USkinnedMeshComponent::UpdateSlaveComponent()
{
	MarkRenderDynamicDataDirty();
}

int32 USkinnedMeshComponent::GetNumMaterials() const
{
	return SkeletalMesh ? SkeletalMesh->Materials.Num() : 0;
}

UMaterialInterface* USkinnedMeshComponent::GetMaterial(int32 MaterialIndex) const
{
	if(MaterialIndex < Materials.Num() && Materials[MaterialIndex])
	{
		return Materials[MaterialIndex];
	}
	else if (SkeletalMesh && MaterialIndex < SkeletalMesh->Materials.Num() && SkeletalMesh->Materials[MaterialIndex].MaterialInterface)
	{
		return SkeletalMesh->Materials[MaterialIndex].MaterialInterface;
	}

	return NULL;
}


bool USkinnedMeshComponent::ShouldCPUSkin()
{
	return false;
}

void USkinnedMeshComponent::GetStreamingTextureInfo(TArray<FStreamingTexturePrimitiveInfo>& OutStreamingTextures) const
{
	if( SkeletalMesh )
	{
		const float LocalTexelFactor = SkeletalMesh->GetStreamingTextureFactor(0) * StreamingDistanceMultiplier;
		const float WorldTexelFactor = LocalTexelFactor * ComponentToWorld.GetMaximumAxisScale();
		const int32 NumMaterials = FMath::Max(SkeletalMesh->Materials.Num(), Materials.Num());
		for( int32 MatIndex = 0; MatIndex < NumMaterials; MatIndex++ )
		{
			UMaterialInterface* const MaterialInterface = GetMaterial(MatIndex);
			if(MaterialInterface)
			{
				TArray<UTexture*> Textures;
				
				MaterialInterface->GetUsedTextures(Textures, EMaterialQualityLevel::Num, false);
				for(int32 TextureIndex = 0; TextureIndex < Textures.Num(); TextureIndex++ )
				{
					FStreamingTexturePrimitiveInfo& StreamingTexture = *new(OutStreamingTextures) FStreamingTexturePrimitiveInfo;
					StreamingTexture.Bounds = Bounds.GetSphere();
					StreamingTexture.TexelFactor = WorldTexelFactor;
					StreamingTexture.Texture = Textures[TextureIndex];
				}
			}
		}
	}
}

bool USkinnedMeshComponent::ShouldUpdateBoneVisibility() const
{
	// do not update if it has MasterPoseComponent
	return !MasterPoseComponent;
}
void USkinnedMeshComponent::RebuildVisibilityArray()
{
	// BoneVisibility needs update if MasterComponent == NULL
	// if MaterComponent, it should follow MaterPoseComponent
	if ( ShouldUpdateBoneVisibility())
	{
		// If the BoneVisibilityStates array has a 0 for a parent bone, all children bones are meant to be hidden as well
		// (as the concatenated matrix will have scale 0).  This code propagates explicitly hidden parents to children.

		// On the first read of any cell of BoneVisibilityStates, BVS_HiddenByParent and BVS_Visible are treated as visible.
		// If it starts out visible, the value written back will be BVS_Visible if the parent is visible; otherwise BVS_HiddenByParent.
		// If it starts out hidden, the BVS_ExplicitlyHidden value stays in place

		// The following code relies on a complete hierarchy sorted from parent to children
		check(BoneVisibilityStates.Num() == SkeletalMesh->RefSkeleton.GetNum());
		for (int32 BoneId=0; BoneId < BoneVisibilityStates.Num(); ++BoneId)
		{
			uint8 VisState = BoneVisibilityStates[BoneId];

			// if not exclusively hidden, consider if parent is hidden
			if (VisState != BVS_ExplicitlyHidden)
			{
				// Check direct parent (only need to do one deep, since we have already processed the parent and written to BoneVisibilityStates previously)
				const int32 ParentIndex = SkeletalMesh->RefSkeleton.GetParentIndex(BoneId);
				if ((ParentIndex == -1) || (BoneVisibilityStates[ParentIndex] == BVS_Visible))
				{
					BoneVisibilityStates[BoneId] = BVS_Visible;
				}
				else
				{
					BoneVisibilityStates[BoneId] = BVS_HiddenByParent;
				}
			}
		}
	}
}

FBoxSphereBounds USkinnedMeshComponent::CalcBounds(const FTransform & LocalToWorld) const
{
	return CalcMeshBound( FVector::ZeroVector, false, LocalToWorld );
}


class UPhysicsAsset* USkinnedMeshComponent::GetPhysicsAsset() const
{
	if (PhysicsAssetOverride)
	{
		return PhysicsAssetOverride;
	}

	if (SkeletalMesh && SkeletalMesh->PhysicsAsset)
	{
		return SkeletalMesh->PhysicsAsset;
	}

	return NULL;
}

FBoxSphereBounds USkinnedMeshComponent::CalcMeshBound(const FVector & RootOffset, bool UsePhysicsAsset, const FTransform & LocalToWorld) const
{
	SCOPE_CYCLE_COUNTER(STAT_UpdateSkelMeshBounds);

	FBoxSphereBounds NewBounds;

	// If physics are asleep, and actor is using physics to move, skip updating the bounds.
	AActor* Owner = GetOwner();
	FVector DrawScale = LocalToWorld.GetScale3D();	

	UPhysicsAsset * const PhysicsAsset = GetPhysicsAsset();
	UPhysicsAsset * const MasterPhysicsAsset = (MasterPoseComponent != NULL)? MasterPoseComponent->GetPhysicsAsset() : NULL;

	// Can only use the PhysicsAsset to calculate the bounding box if we are not non-uniformly scaling the mesh.
	const bool bCanUsePhysicsAsset = DrawScale.IsUniform() && (SkeletalMesh != NULL)
		// either space base exists or child component
		&& ( (SpaceBases.Num() == SkeletalMesh->RefSkeleton.GetNum()) || (MasterPhysicsAsset) );

	const bool bDetailModeAllowsRendering = (DetailMode <= GetCachedScalabilityCVars().DetailMode);
	const bool bVisible = ( bDetailModeAllowsRendering && (ShouldRender() || bCastHiddenShadow));

	const bool bHasPhysBodies = PhysicsAsset && PhysicsAsset->BodySetup.Num();
	const bool bMasterHasPhysBodies = MasterPhysicsAsset && MasterPhysicsAsset->BodySetup.Num();

	// if not visible, or we were told to use fixed bounds, use skelmesh bounds
	if ( (!bVisible || bComponentUseFixedSkelBounds) && SkeletalMesh ) 
	{
		FBoxSphereBounds RootAdjustedBounds = SkeletalMesh->Bounds;
		RootAdjustedBounds.Origin += RootOffset; // Adjust bounds by root bone translation
		NewBounds = RootAdjustedBounds.TransformBy(LocalToWorld);
	}
	else if( MasterPoseComponent && MasterPoseComponent->SkeletalMesh && MasterPoseComponent->bComponentUseFixedSkelBounds )
	{
		FBoxSphereBounds RootAdjustedBounds = MasterPoseComponent->SkeletalMesh->Bounds;
		RootAdjustedBounds.Origin += RootOffset; // Adjust bounds by root bone translation
		NewBounds = RootAdjustedBounds.TransformBy(LocalToWorld);
	}
#if WITH_EDITOR
	// For AnimSet Viewer, use 'bounds preview' physics asset if present.
	else if(SkeletalMesh && bHasPhysBodies && bCanUsePhysicsAsset)
	{
		// @fixme UE4 this does not use LocalToWorld entered but ComponentToWorld
		NewBounds = FBoxSphereBounds(PhysicsAsset->CalcAABB(this));
	}
#endif // WITH_EDITOR
	// If we have a PhysicsAsset (with at least one matching bone), and we can use it, do so to calc bounds.
	else if( bHasPhysBodies && bCanUsePhysicsAsset && UsePhysicsAsset )
	{
		// @fixme UE4 this does not use LocalToWorld entered but ComponentToWorld
		NewBounds = FBoxSphereBounds(PhysicsAsset->CalcAABB(this));
	}
	// Use MasterPoseComponent's PhysicsAsset, if we don't have one and it does
	else if( MasterPoseComponent && bCanUsePhysicsAsset && bUseBoundsFromMasterPoseComponent )
	{
		NewBounds = MasterPoseComponent->Bounds;
	}
	else if (MasterPoseComponent && bCanUsePhysicsAsset && bMasterHasPhysBodies)
	{
		// @fixme UE4 this does not use LocalToWorld entered but ComponentToWorld		
		NewBounds = FBoxSphereBounds(MasterPhysicsAsset->CalcAABB(this));
	}
	// Fallback is to use the one from the skeletal mesh. Usually pretty bad in terms of Accuracy of where the SkelMesh Bounds are located (i.e. usually bigger than it needs to be)
	else if( SkeletalMesh )
	{
		FBoxSphereBounds RootAdjustedBounds = SkeletalMesh->Bounds;

		// Adjust bounds by root bone translation
		RootAdjustedBounds.Origin += RootOffset;
		NewBounds = RootAdjustedBounds.TransformBy(LocalToWorld);
	}
	else
	{
		NewBounds = FBoxSphereBounds(LocalToWorld.GetLocation(), FVector::ZeroVector, 0.f);
	}

	// Add bounds of any per-poly collision data.
	// TODO UE4

	NewBounds.BoxExtent *= BoundsScale;
	NewBounds.SphereRadius *= BoundsScale;

	return NewBounds;
}


FMatrix USkinnedMeshComponent::GetBoneMatrix(int32 BoneIdx) const
{
	if ( !IsRegistered() )
	{
		// if not registered, we don't have SpaceBases yet. 
		// also ComponentToWorld isn't set yet (They're set from relativetranslation, relativerotation, relativescale)
		return FMatrix::Identity;
	}

	// Handle case of use a MasterPoseComponent - get bone matrix from there.
	if(MasterPoseComponent)
	{
		if(BoneIdx < MasterBoneMap.Num())
		{
			int32 ParentBoneIndex = MasterBoneMap[BoneIdx];

			// If ParentBoneIndex is valid, grab matrix from MasterPoseComponent.
			if(	ParentBoneIndex != INDEX_NONE && 
				ParentBoneIndex < MasterPoseComponent->SpaceBases.Num())
			{
				return MasterPoseComponent->SpaceBases[ParentBoneIndex].ToMatrixWithScale() * ComponentToWorld.ToMatrixWithScale();
			}
			else
			{
				UE_LOG(LogSkinnedMeshComp, Warning, TEXT("GetBoneMatrix : ParentBoneIndex(%d) out of range of MasterPoseComponent->SpaceBases for %s"), BoneIdx, *GetPathName());
				return FMatrix::Identity;
			}
		}
		else
		{
			UE_LOG(LogSkinnedMeshComp, Warning, TEXT("GetBoneMatrix : BoneIndex(%d) out of range of MasterBoneMap for %s (%s)"), BoneIdx, *this->GetFName().ToString(), SkeletalMesh ? *SkeletalMesh->GetFName().ToString() : TEXT("NULL") );
			return FMatrix::Identity;
		}
	}
	else
	{
		if( SpaceBases.Num() && BoneIdx < SpaceBases.Num() )
		{
			return SpaceBases[BoneIdx].ToMatrixWithScale() * ComponentToWorld.ToMatrixWithScale();
		}
		else
		{
			UE_LOG(LogSkinnedMeshComp, Warning, TEXT("GetBoneMatrix : BoneIndex(%d) out of range of SpaceBases for %s (%s)"), BoneIdx, *GetPathName(), SkeletalMesh ? *SkeletalMesh->GetFullName() : TEXT("NULL") );
			return FMatrix::Identity;
		}
	}
}


FTransform USkinnedMeshComponent::GetBoneTransform(int32 BoneIdx) const
{
	if ( !IsRegistered() )
	{
		// if not registered, we don't have SpaceBases yet. 
		// also ComponentToWorld isn't set yet (They're set from relativelocation, relativerotation, relativescale)
		return FTransform::Identity;
	}

	// Handle case of use a MasterPoseComponent - get bone matrix from there.
	if(MasterPoseComponent)
	{
		if(BoneIdx < MasterBoneMap.Num())
		{
			int32 ParentBoneIndex = MasterBoneMap[BoneIdx];

			// If ParentBoneIndex is valid, grab matrix from MasterPoseComponent.
			if(	ParentBoneIndex != INDEX_NONE && 
				ParentBoneIndex < MasterPoseComponent->SpaceBases.Num())
			{
				return MasterPoseComponent->SpaceBases[ParentBoneIndex] * ComponentToWorld;
			}
			else
			{
				UE_LOG(LogSkinnedMeshComp, Warning, TEXT("GetBoneTransform : ParentBoneIndex(%d) out of range of MasterPoseComponent->SpaceBases for %s"), BoneIdx, *this->GetFName().ToString() );
				return FTransform::Identity;
			}
		}
		else
		{
			UE_LOG(LogSkinnedMeshComp, Warning, TEXT("GetBoneTransform : BoneIndex(%d) out of range of MasterBoneMap for %s"), BoneIdx, *this->GetFName().ToString() );
			return FTransform::Identity;
		}
	}
	else
	{
		if( SpaceBases.Num() && BoneIdx < SpaceBases.Num() )
		{
			return SpaceBases[BoneIdx] * ComponentToWorld;
		}
		else
		{
			UE_LOG(LogSkinnedMeshComp, Warning, TEXT("GetBoneTransform : BoneIndex(%d) out of range of SpaceBases for %s (%s)"), BoneIdx, *GetPathName(), SkeletalMesh ? *SkeletalMesh->GetFullName() : TEXT("NULL") );
			return FTransform::Identity;
		}
	}
}


int32 USkinnedMeshComponent::GetBoneIndex( FName BoneName) const
{
	int32 BoneIndex = INDEX_NONE;
	if ( BoneName != NAME_None && SkeletalMesh )
	{
		BoneIndex = SkeletalMesh->RefSkeleton.FindBoneIndex( BoneName );
	}

	return BoneIndex;
}


FName USkinnedMeshComponent::GetBoneName(int32 BoneIndex) const
{
	return (SkeletalMesh != NULL && SkeletalMesh->RefSkeleton.IsValidIndex(BoneIndex)) ? SkeletalMesh->RefSkeleton.GetBoneName(BoneIndex) : NAME_None;
}


FName USkinnedMeshComponent::GetParentBone( FName BoneName ) const
{
	FName Result = NAME_None;

	int32 BoneIndex = GetBoneIndex(BoneName);
	if ((BoneIndex != INDEX_NONE) && (BoneIndex > 0)) // This checks that this bone is not the root (ie no parent), and that BoneIndex != INDEX_NONE (ie bone name was found)
	{
		Result = SkeletalMesh->RefSkeleton.GetBoneName(SkeletalMesh->RefSkeleton.GetParentIndex(BoneIndex));
	}
	return Result;
}


void USkinnedMeshComponent::GetBoneNames(TArray<FName>& BoneNames)
{
	if (SkeletalMesh == NULL)
	{
		// no mesh, so no bones
		BoneNames.Empty();
	}
	else
	{
		// pre-size the array to avoid unnecessary reallocation
		BoneNames.Empty(SkeletalMesh->RefSkeleton.GetNum());
		BoneNames.AddUninitialized(SkeletalMesh->RefSkeleton.GetNum());
		for (int32 i = 0; i < SkeletalMesh->RefSkeleton.GetNum(); i++)
		{
			BoneNames[i] = SkeletalMesh->RefSkeleton.GetBoneName(i);
		}
	}
}


bool USkinnedMeshComponent::BoneIsChildOf(FName BoneName, FName ParentBoneName) const
{
	bool bResult = false;

	if( SkeletalMesh )
	{
		const int32 BoneIndex = SkeletalMesh->RefSkeleton.FindBoneIndex(BoneName);
		if(BoneIndex == INDEX_NONE)
		{
			UE_LOG(LogSkinnedMeshComp, Log, TEXT("execBoneIsChildOf: BoneName '%s' not found in SkeletalMesh '%s'"), *BoneName.ToString(), *SkeletalMesh->GetName());
			return bResult;
		}

		const int32 ParentBoneIndex = SkeletalMesh->RefSkeleton.FindBoneIndex(ParentBoneName);
		if(ParentBoneIndex == INDEX_NONE)
		{
			UE_LOG(LogSkinnedMeshComp, Log, TEXT("execBoneIsChildOf: ParentBoneName '%s' not found in SkeletalMesh '%s'"), *ParentBoneName.ToString(), *SkeletalMesh->GetName());
			return bResult;
		}

		bResult = SkeletalMesh->RefSkeleton.BoneIsChildOf(BoneIndex, ParentBoneIndex);
	}

	return bResult;
}


FVector USkinnedMeshComponent::GetRefPosePosition(int32 BoneIndex)
{
	if(SkeletalMesh && (BoneIndex >= 0) && (BoneIndex < SkeletalMesh->RefSkeleton.GetNum()))
	{
		return SkeletalMesh->RefSkeleton.GetRefBonePose()[BoneIndex].GetTranslation();
	}
	else
	{
		return FVector::ZeroVector;
	}
}


void USkinnedMeshComponent::SetSkeletalMesh(USkeletalMesh* InSkelMesh)
{
	// NOTE: InSkelMesh may be NULL (useful in the editor for removing the skeletal mesh associated with
	//   this component on-the-fly)

	if (InSkelMesh == SkeletalMesh)
	{
		// do nothing if the input mesh is the same mesh we're already using.
		return;
	}

	// Unregister the component, swap the skeletal mesh, and reregister.
	{
		FComponentReregisterContext ReregisterContext(this);
		check(MeshObject == NULL);
		SkeletalMesh = InSkelMesh;
	}
	
	// TODO: (LH) find better way to call this 
	// Right now SkeletalMeshComponent(child) needs to call this after animation update
	// But some of functions of SkinnedMeshComponent has to be called before
	// So it's hard to split both separate, but do we really need to call this when skeletalmesh is set
	// Think about this
	// RefreshBoneTransforms(); 
	
	// Notify the streaming system. Don't use Update(), because this may be the first time the mesh has been set
	// and the component may have to be added to the streaming system for the first time.
	GStreamingManager->NotifyPrimitiveAttached( this, DPT_Spawned );
}

FSkeletalMeshResource* USkinnedMeshComponent::GetSkeletalMeshResource() const
{
	if (MeshObject)
	{
		return &MeshObject->GetSkeletalMeshResource();
	}
	else if (SkeletalMesh)
	{
		ERHIFeatureLevel::Type SceneFeatureLevel = GRHIFeatureLevel;
		return SkeletalMesh->GetResourceForRendering(SceneFeatureLevel);
	}
	else
	{
		return NULL;
	}
}

bool USkinnedMeshComponent::AllocateTransformData()
{
	// Allocate transforms if not present.
	if ( SkeletalMesh != NULL && MasterPoseComponent == NULL )
	{
		if( SpaceBases.Num() != SkeletalMesh->RefSkeleton.GetNum() )
		{
			SpaceBases.Empty( SkeletalMesh->RefSkeleton.GetNum() );
			SpaceBases.AddUninitialized( SkeletalMesh->RefSkeleton.GetNum() );

			for(int32 I=0; I<SkeletalMesh->RefSkeleton.GetNum(); ++I)
			{
				SpaceBases[I].SetIdentity();
			}
 
			BoneVisibilityStates.Empty( SkeletalMesh->RefSkeleton.GetNum() );
			if( SkeletalMesh->RefSkeleton.GetNum() )
			{
				BoneVisibilityStates.AddUninitialized( SkeletalMesh->RefSkeleton.GetNum() );
				for (int32 BoneIndex = 0; BoneIndex < SkeletalMesh->RefSkeleton.GetNum(); BoneIndex++)
				{
					BoneVisibilityStates[ BoneIndex ] = BVS_Visible;
				}
			}
		}

		// if it's same, do not touch, and return
		return true;
	}
	
	// Reset the animation stuff when changing mesh.
	SpaceBases.Empty();		

	return false;
}

void USkinnedMeshComponent::DeallocateTransformData()
{
	SpaceBases.Empty();
	BoneVisibilityStates.Empty();
}

void USkinnedMeshComponent::SetPhysicsAsset(class UPhysicsAsset* InPhysicsAsset, bool bForceReInit)
{
	PhysicsAssetOverride = InPhysicsAsset;
}


void USkinnedMeshComponent::SetMasterPoseComponent(class USkinnedMeshComponent* NewMasterBoneComponent)
{
	MasterPoseComponent = NewMasterBoneComponent;

	// now add to slave components list, 
	if (MasterPoseComponent)
	{
		bool bAddNew=true;
		// make sure no empty element is there, this is weak obj ptr, so it will go away unless there is 
		// other reference, this is intentional as master to slave reference is weak
		for ( auto Iter = MasterPoseComponent->SlavePoseComponents.CreateIterator(); Iter; ++Iter )
		{
			TWeakObjectPtr<USkinnedMeshComponent> Comp = (*Iter);
			if (Comp.IsValid() == false)
			{
				// remove
				MasterPoseComponent->SlavePoseComponents.RemoveAt(Iter.GetIndex());
				--Iter;
			}
			// if it has same as me, ignore to add
			else if (Comp.Get() == this)
			{
				bAddNew	= false;
			}
		}

		if (bAddNew)
		{
			MasterPoseComponent->SlavePoseComponents.Add(this);
		}
	}

	AllocateTransformData();
	UpdateMasterBoneMap();
}

void USkinnedMeshComponent::InvalidateCachedBounds()
{
	bCachedLocalBoundsUpToDate = false;

	// Also invalidate all slave components.
	if( SlavePoseComponents.Num() > 0 )
	{
		for (auto Iter=SlavePoseComponents.CreateIterator(); Iter; ++Iter)
		{
			TWeakObjectPtr<USkinnedMeshComponent> SkinnedMeshComp = (*Iter);
			if( SkinnedMeshComp.IsValid() )
			{
				SkinnedMeshComp->bCachedLocalBoundsUpToDate = false;
			}
		}
	}
}

void USkinnedMeshComponent::RefreshSlaveComponents()
{
	if ( SlavePoseComponents.Num() > 0 )
	{
		for (auto Iter=SlavePoseComponents.CreateIterator(); Iter; ++Iter)
		{
			TWeakObjectPtr<USkinnedMeshComponent> MeshComp = (*Iter);
			if (MeshComp.IsValid())
			{
				MeshComp->MarkRenderDynamicDataDirty();
			}
		}
	}
}

void USkinnedMeshComponent::SetForceWireframe(bool InForceWireframe)
{
	if(bForceWireframe != InForceWireframe)
	{
		bForceWireframe = InForceWireframe;
		MarkRenderStateDirty();
	}
}


UMorphTarget* USkinnedMeshComponent::FindMorphTarget( FName MorphTargetName )
{
	if( SkeletalMesh != NULL )
	{
		return SkeletalMesh->FindMorphTarget(MorphTargetName);
	}

	return NULL;
}


void USkinnedMeshComponent::UpdateMasterBoneMap()
{
	MasterBoneMap.Empty();

	if(SkeletalMesh && MasterPoseComponent && MasterPoseComponent->SkeletalMesh)
	{
		USkeletalMesh* ParentMesh = MasterPoseComponent->SkeletalMesh;

		MasterBoneMap.Empty( SkeletalMesh->RefSkeleton.GetNum() );
		MasterBoneMap.AddUninitialized( SkeletalMesh->RefSkeleton.GetNum() );
		if (SkeletalMesh == ParentMesh)
		{
			// if the meshes are the same, the indices must match exactly so we don't need to look them up
			for (int32 i = 0; i < MasterBoneMap.Num(); i++)
			{
				MasterBoneMap[i] = i;
			}
		}
		else
		{
			for(int32 i=0; i<MasterBoneMap.Num(); i++)
			{
				FName BoneName = SkeletalMesh->RefSkeleton.GetBoneName(i);
				MasterBoneMap[i] = ParentMesh->RefSkeleton.FindBoneIndex( BoneName );
			}
		}
	}
}

TArray<FName> USkinnedMeshComponent::GetAllSocketNames() const
{
	TArray<FName> SocketNames;
	if( SkeletalMesh )
	{
		for( auto It=SkeletalMesh->Skeleton->Sockets.CreateConstIterator(); It; ++It )
		{
			SocketNames.Add( FName( *((*It)->GetName()) ) );
		}
	}
	return SocketNames;
}

FTransform USkinnedMeshComponent::GetSocketTransform(FName InSocketName, ERelativeTransformSpace TransformSpace) const
{
	FTransform OutSocketTransform = ComponentToWorld;

	if (InSocketName != NAME_None)
	{
		int32 BoneIndex = GetBoneIndex(InSocketName);
		if( BoneIndex != INDEX_NONE )
		{
			OutSocketTransform = GetBoneTransform(BoneIndex);
		}
		else
		{
			USkeletalMeshSocket const* const Socket = GetSocketByName(InSocketName);
			if (Socket)
			{
				FTransform SocketLocalTransform = Socket->GetSocketLocalTransform();

				BoneIndex = GetBoneIndex(Socket->BoneName);
				if( BoneIndex != INDEX_NONE )
				{
					FTransform BoneTransform = GetBoneTransform(BoneIndex);
					OutSocketTransform = SocketLocalTransform * BoneTransform; 
				}
			}
		}
	}

	switch(TransformSpace)
	{
		case RTS_Actor:
		{
			if( AActor* Actor = GetOwner() )
			{
				return OutSocketTransform.GetRelativeTransform( Actor->GetTransform() );
			}
			break;
		}
		case RTS_Component:
		{
			return OutSocketTransform.GetRelativeTransform( ComponentToWorld );
		}
	}

	return OutSocketTransform;
}

class USkeletalMeshSocket const* USkinnedMeshComponent::GetSocketByName(FName InSocketName) const
{
	USkeletalMeshSocket const* Socket = NULL;

	if( SkeletalMesh )
	{
		Socket = SkeletalMesh->FindSocket( InSocketName );
	}
	else
	{
		UE_LOG(LogSkinnedMeshComp, Warning,TEXT("GetSocketByName(): No SkeletalMesh for %s"), *GetName());
	}

	return Socket;
}

bool USkinnedMeshComponent::DoesSocketExist(FName InSocketName) const
{
	return (GetSocketByName(InSocketName) != NULL);
}

FName USkinnedMeshComponent::GetSocketBoneName(FName InSocketName)
{
	if(!SkeletalMesh)
	{
		return NAME_None;
	}

	// First check for a socket
	USkeletalMeshSocket const* TmpSocket = SkeletalMesh->FindSocket(InSocketName);
	if( TmpSocket )
	{
		return TmpSocket->BoneName;
	}

	// If socket is not found, maybe it was just a bone name.
	if( GetBoneIndex(InSocketName) != INDEX_NONE )
	{
		return InSocketName;
	}

	// Doesn't exist.
	return NAME_None;
}


FQuat USkinnedMeshComponent::GetBoneQuaternion(FName BoneName, int32 Space) const
{
	int32 BoneIndex = GetBoneIndex(BoneName);

	if( BoneIndex == INDEX_NONE )
	{
		UE_LOG(LogSkinnedMeshComp, Warning, TEXT("USkinnedMeshComponent::execGetBoneQuaternion : Could not find bone: %s"), *BoneName.ToString());
		return FQuat::Identity;
	}

	FTransform BoneTransform;
	if( Space == 1 )
	{
		if(MasterPoseComponent)
		{
			if(BoneIndex < MasterBoneMap.Num())
			{
				int32 ParentBoneIndex = MasterBoneMap[BoneIndex];
				// If ParentBoneIndex is valid, grab matrix from MasterPoseComponent.
				if(	ParentBoneIndex != INDEX_NONE && 
					ParentBoneIndex < MasterPoseComponent->SpaceBases.Num())
				{
					BoneTransform = MasterPoseComponent->SpaceBases[ParentBoneIndex];
				}
				else
				{
					BoneTransform = FTransform::Identity;
				}
			}
			else
			{
				BoneTransform = FTransform::Identity;
			}
		}
		else
		{
			BoneTransform = SpaceBases[BoneIndex];
		}
	}
	else
	{
		BoneTransform = GetBoneTransform(BoneIndex);
	}

	BoneTransform.RemoveScaling();
	return BoneTransform.GetRotation();
}


FVector USkinnedMeshComponent::GetBoneLocation( FName BoneName, int32 Space ) const
{
	int32 BoneIndex = GetBoneIndex(BoneName);
	if( BoneIndex == INDEX_NONE )
	{
		UE_LOG(LogAnimation, Log, TEXT("USkinnedMeshComponent::GetBoneLocation (%s %s): Could not find bone: %s"), *GetFullName(), *GetDetailedInfo(), *BoneName.ToString() );
		return FVector::ZeroVector;
	}

	// If space == Local
	if( Space == 1 )
	{
		if(MasterPoseComponent)
		{
			if(BoneIndex < MasterBoneMap.Num())
			{
				int32 ParentBoneIndex = MasterBoneMap[BoneIndex];
				// If ParentBoneIndex is valid, grab transform from MasterPoseComponent.
				if(	ParentBoneIndex != INDEX_NONE && 
					ParentBoneIndex < MasterPoseComponent->SpaceBases.Num())
				{
					return MasterPoseComponent->SpaceBases[ParentBoneIndex].GetLocation();
				}
			}
			
			// return empty vector
			return FVector::ZeroVector;			
		}
		else
		{
			return SpaceBases[BoneIndex].GetLocation();
		}
	}
	else
	{
		// To support non-uniform scale (via LocalToWorld), use GetBoneMatrix
		return GetBoneMatrix(BoneIndex).GetOrigin();
	}
}


FVector USkinnedMeshComponent::GetBoneAxis( FName BoneName, EAxis::Type Axis ) const
{
	int32 BoneIndex = GetBoneIndex(BoneName);
	if (BoneIndex == INDEX_NONE)
	{
		UE_LOG(LogSkinnedMeshComp, Warning, TEXT("USkinnedMeshComponent::execGetBoneAxis : Could not find bone: %s"), *BoneName.ToString());
		return FVector::ZeroVector;
	}
	else if (Axis == EAxis::None)
	{
		UE_LOG(LogSkinnedMeshComp, Warning, TEXT("USkinnedMeshComponent::execGetBoneAxis: Invalid axis specified"));
		return FVector::ZeroVector;
	}
	else
	{
		return GetBoneMatrix(BoneIndex).GetUnitAxis(Axis);
	}
}

bool USkinnedMeshComponent::HasAnySockets() const
{
	return (SkeletalMesh != NULL) && (
#if WITH_EDITOR
		(SkeletalMesh->GetActiveSocketList().Num() > 0) ||
#endif
		(SkeletalMesh->RefSkeleton.GetNum() > 0));
}

void USkinnedMeshComponent::QuerySupportedSockets(TArray<FComponentSocketDescription>& OutSockets) const
{
	if (SkeletalMesh != NULL)
	{
		//@TODO: need to make this work in the game too
#if WITH_EDITOR
		// Grab all the mesh and skeleton sockets
		const TArray<USkeletalMeshSocket*> AllSockets = SkeletalMesh->GetActiveSocketList();

		for (int32 SocketIdx = 0; SocketIdx < AllSockets.Num(); ++SocketIdx)
		{
			if (USkeletalMeshSocket* Socket = AllSockets[SocketIdx])
			{
				new (OutSockets) FComponentSocketDescription(Socket->SocketName, EComponentSocketType::Socket);
			}
		}
#endif

		// Now grab the bones, which can behave exactly like sockets
		for (int32 BoneIdx = 0; BoneIdx < SkeletalMesh->RefSkeleton.GetNum(); ++BoneIdx)
		{
			const FName BoneName = SkeletalMesh->RefSkeleton.GetBoneName(BoneIdx);
			new (OutSockets) FComponentSocketDescription(BoneName, EComponentSocketType::Bone);
		}
	}
}

void USkinnedMeshComponent::UpdateOverlaps(TArray<FOverlapInfo> const* PendingOverlaps, bool bDoNotifies, const TArray<FOverlapInfo>* OverlapsAtEndLocation)
{
	// we don't support overlap test on destructible or physics asset
	// so use SceneComponent::UpdateOverlaps to handle children
	USceneComponent::UpdateOverlaps(PendingOverlaps, bDoNotifies, OverlapsAtEndLocation);
}

void USkinnedMeshComponent::TransformToBoneSpace(FName BoneName, FVector InPosition, FRotator InRotation, FVector& OutPosition, FRotator& OutRotation) const
{
	int32 BoneIndex = GetBoneIndex(BoneName);
	if(BoneIndex != INDEX_NONE)
	{
		FMatrix BoneToWorldTM = GetBoneMatrix(BoneIndex);
		FMatrix WorldTM = FRotationTranslationMatrix(InRotation, InPosition);
		FMatrix LocalTM = WorldTM * BoneToWorldTM.InverseSafe();

		OutPosition = LocalTM.GetOrigin();
		OutRotation = LocalTM.Rotator();
	}
}


void USkinnedMeshComponent::TransformFromBoneSpace(FName BoneName, FVector InPosition, FRotator InRotation, FVector& OutPosition, FRotator& OutRotation)
{
	int32 BoneIndex = GetBoneIndex(BoneName);
	if(BoneIndex != INDEX_NONE)
	{
		FMatrix BoneToWorldTM = GetBoneMatrix(BoneIndex);

		FMatrix LocalTM = FRotationTranslationMatrix(InRotation, InPosition);
		FMatrix WorldTM = LocalTM * BoneToWorldTM;

		OutPosition = WorldTM.GetOrigin();
		OutRotation = WorldTM.Rotator();
	}
}



FName USkinnedMeshComponent::FindClosestBone(FVector TestLocation, FVector* BoneLocation, float IgnoreScale) const
{
	if (SkeletalMesh == NULL)
	{
		if (BoneLocation != NULL)
		{
			*BoneLocation = FVector::ZeroVector;
		}
		return NAME_None;
	}
	else
	{
		// transform the TestLocation into mesh local space so we don't have to transform the (mesh local) bone locations
		TestLocation = ComponentToWorld.InverseTransformPosition(TestLocation);
		
		float IgnoreScaleSquared = FMath::Square(IgnoreScale);
		float BestDistSquared = BIG_NUMBER;
		int32 BestIndex = -1;
		for (int32 i = 0; i < SpaceBases.Num(); i++)
		{
			if (IgnoreScale < 0.f || SpaceBases[i].GetScaledAxis( EAxis::X ).SizeSquared() > IgnoreScaleSquared)
			{
				float DistSquared = (TestLocation - SpaceBases[i].GetLocation()).SizeSquared();
				if (DistSquared < BestDistSquared)
				{
					BestIndex = i;
					BestDistSquared = DistSquared;
				}
			}
		}

		if (BestIndex == -1)
		{
			if (BoneLocation != NULL)
			{
				*BoneLocation = FVector::ZeroVector;
			}
			return NAME_None;
		}
		else
		{
			// transform the bone location into world space
			if (BoneLocation != NULL)
			{
				*BoneLocation = (SpaceBases[BestIndex] * ComponentToWorld).GetLocation();
			}
			return SkeletalMesh->RefSkeleton.GetBoneName(BestIndex);
		}
	}
}

void USkinnedMeshComponent::ShowMaterialSection(int32 MaterialID, bool bShow, int32 LODIndex)
{
	if (!SkeletalMesh)
	{
		// no skeletalmesh, then nothing to do. 
		return;
	}
	// Make sure LOD info for this component has been initialized
	InitLODInfos();
	if (LODInfo.IsValidIndex(LODIndex))
	{
		const FSkeletalMeshLODInfo& SkelLODInfo = SkeletalMesh->LODInfo[LODIndex];
		FSkelMeshComponentLODInfo& SkelCompLODInfo = LODInfo[LODIndex];
		TArray<bool>& HiddenMaterials = SkelCompLODInfo.HiddenMaterials;
	
		// allocate if not allocated yet
		if ( HiddenMaterials.Num() != SkeletalMesh->Materials.Num() )
		{
			// Using skeletalmesh component because Materials.Num() should be <= SkeletalMesh->Materials.Num()		
			HiddenMaterials.Empty(SkeletalMesh->Materials.Num());
			HiddenMaterials.AddZeroed(SkeletalMesh->Materials.Num());
		}
		// If we are at a dropped LOD, route material index through the LODMaterialMap in the LODInfo struct.
		int32 UseMaterialIndex = MaterialID;			
		if(LODIndex > 0)
		{
			if(SkelLODInfo.LODMaterialMap.IsValidIndex(MaterialID))
			{
				UseMaterialIndex = SkelLODInfo.LODMaterialMap[MaterialID];
				UseMaterialIndex = FMath::Clamp( UseMaterialIndex, 0, HiddenMaterials.Num() );
			}
		}
		// Mark the mapped section material entry as visible/hidden
		if (HiddenMaterials.IsValidIndex(UseMaterialIndex))
		{
			HiddenMaterials[UseMaterialIndex] = !bShow;
		}

		if ( MeshObject )
		{
			// need to send render thread for updated hidden section
			ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(
				FUpdateHiddenSectionCommand, 
				FSkeletalMeshObject*, MeshObject, MeshObject, 
				TArray<bool>, HiddenMaterials, HiddenMaterials, 
				int32, LODIndex, LODIndex,
			{
				MeshObject->SetHiddenMaterials(LODIndex,HiddenMaterials);
			});
		}
	}
}


void USkinnedMeshComponent::GetUsedMaterials( TArray<UMaterialInterface*>& OutMaterials ) const
{
	if( SkeletalMesh )
	{
		// The max number of materials used is the max of the materials on the skeletal mesh and the materials on the mesh component
		const int32 NumMaterials = FMath::Max( SkeletalMesh->Materials.Num(), Materials.Num() );
		for( int32 MatIdx = 0; MatIdx < NumMaterials; ++MatIdx )
		{
			// GetMaterial will determine the correct material to use for this index.  
			UMaterialInterface* MaterialInterface = GetMaterial( MatIdx );
			OutMaterials.Add( MaterialInterface );
		}
	}
}

template <bool bExtraBoneInfluencesT>
FORCEINLINE FVector USkinnedMeshComponent::GetTypedSkinnedVertexPosition(const FSkelMeshChunk& Chunk, const FSkeletalMeshVertexBuffer& VertexBufferGPUSkin, int32 VertIndex, bool bSoftVertex) const
{
	FVector SkinnedPos(0,0,0);

	const USkinnedMeshComponent* BaseComponent = MasterPoseComponent ? MasterPoseComponent : this;

	// Do soft skinning for this vertex.
	if(bSoftVertex)
	{
		const TGPUSkinVertexBase<bExtraBoneInfluencesT>* SrcSoftVertex = VertexBufferGPUSkin.GetVertexPtr<bExtraBoneInfluencesT>(Chunk.GetSoftVertexBufferIndex()+VertIndex);

#if !PLATFORM_LITTLE_ENDIAN
		// uint8[] elements in LOD.VertexBufferGPUSkin have been swapped for VET_UBYTE4 vertex stream use
		for(int32 InfluenceIndex = MAX_INFLUENCES-1;InfluenceIndex >=  MAX_INFLUENCES-Chunk.MaxBoneInfluences;InfluenceIndex--)
#else
		for(int32 InfluenceIndex = 0;InfluenceIndex < Chunk.MaxBoneInfluences;InfluenceIndex++)
#endif
		{
			int32 BoneIndex = Chunk.BoneMap[SrcSoftVertex->InfluenceBones[InfluenceIndex]];
			if(MasterPoseComponent)
			{		
				check(MasterBoneMap.Num() == SkeletalMesh->RefSkeleton.GetNum());
				BoneIndex = MasterBoneMap[BoneIndex];
			}

			const float		Weight = (float)SrcSoftVertex->InfluenceWeights[InfluenceIndex] / 255.0f;
			const FMatrix	RefToLocal = SkeletalMesh->RefBasesInvMatrix[BoneIndex] * BaseComponent->SpaceBases[BoneIndex].ToMatrixWithScale();

			SkinnedPos += RefToLocal.TransformPosition(VertexBufferGPUSkin.GetVertexPositionFast(SrcSoftVertex)) * Weight;
		}
	}
	// Do rigid (one-influence) skinning for this vertex.
	else
	{
		const TGPUSkinVertexBase<false>* SrcRigidVertex = VertexBufferGPUSkin.GetVertexPtr<false>(Chunk.GetRigidVertexBufferIndex()+VertIndex);
		const int32 RigidInfluenceIndex = SkinningTools::GetRigidInfluenceIndex();
		int32 BoneIndex = Chunk.BoneMap[SrcRigidVertex->InfluenceBones[RigidInfluenceIndex]];
		if(MasterPoseComponent)
		{
			check(MasterBoneMap.Num() == SkeletalMesh->RefSkeleton.GetNum());
			BoneIndex = MasterBoneMap[BoneIndex];
		}

		const FMatrix	RefToLocal = SkeletalMesh->RefBasesInvMatrix[BoneIndex] * BaseComponent->SpaceBases[BoneIndex].ToMatrixWithScale();

		SkinnedPos = RefToLocal.TransformPosition(VertexBufferGPUSkin.GetVertexPositionFast(SrcRigidVertex));
	}

	return SkinnedPos;
}

FVector USkinnedMeshComponent::GetSkinnedVertexPosition(int32 VertexIndex) const
{
	FVector SkinnedPos(0,0,0);

	// Fail if no mesh
	if(!SkeletalMesh || !MeshObject)
	{
		return SkinnedPos;
	}

	FStaticLODModel& Model = MeshObject->GetSkeletalMeshResource().LODModels[0];

	// Find the chunk and vertex within that chunk, and skinning type, for this vertex.
	int32 ChunkIndex;
	int32 VertIndex;
	bool bSoftVertex;
	bool bHasExtraBoneInfluences;
	Model.GetChunkAndSkinType(VertexIndex, ChunkIndex, VertIndex, bSoftVertex, bHasExtraBoneInfluences);

	check(ChunkIndex < Model.Chunks.Num());
	const FSkelMeshChunk& Chunk = Model.Chunks[ChunkIndex];

	return bHasExtraBoneInfluences
		? GetTypedSkinnedVertexPosition<true>(Chunk, Model.VertexBufferGPUSkin, VertIndex, bSoftVertex)
		: GetTypedSkinnedVertexPosition<false>(Chunk, Model.VertexBufferGPUSkin, VertIndex, bSoftVertex);
}


void USkinnedMeshComponent::HideBone( int32 BoneIndex, EPhysBodyOp PhysBodyOption)
{
	if (ShouldUpdateBoneVisibility() && BoneIndex < BoneVisibilityStates.Num())
	{
		checkSlow ( BoneIndex != INDEX_NONE );
		BoneVisibilityStates[ BoneIndex ] = BVS_ExplicitlyHidden;
		RebuildVisibilityArray();
	}
}


void USkinnedMeshComponent::UnHideBone( int32 BoneIndex )
{
	if (ShouldUpdateBoneVisibility() && BoneIndex < BoneVisibilityStates.Num())
	{
		checkSlow ( BoneIndex != INDEX_NONE );
		//@TODO: If unhiding the child of a still hidden bone (coming in, BoneVisibilityStates(RefSkel(BoneIndex).ParentIndex) != BVS_Visible),
		// should we be re-enabling collision bodies?
		// Setting visible to true here is OK in either case as it will be reset to BVS_HiddenByParent in RecalcRequiredBones later if needed.
		BoneVisibilityStates[ BoneIndex ] = BVS_Visible;
		RebuildVisibilityArray();
	}
}


bool USkinnedMeshComponent::IsBoneHidden( int32 BoneIndex ) const
{
	if (ShouldUpdateBoneVisibility())
	{
		if ( BoneIndex != INDEX_NONE )
		{
			return BoneVisibilityStates[ BoneIndex ] != BVS_Visible;
		}
	}
	else if ( MasterPoseComponent != NULL )
	{
		return MasterPoseComponent->IsBoneHidden( BoneIndex );
	}

	return false;
}


void USkinnedMeshComponent::HideBoneByName( FName BoneName, EPhysBodyOp PhysBodyOption )
{
	// Find appropriate BoneIndex
	int32 BoneIndex = GetBoneIndex(BoneName);
	if ( BoneIndex != INDEX_NONE )
	{
		HideBone(BoneIndex, PhysBodyOption);
	}
}


void USkinnedMeshComponent::UnHideBoneByName( FName BoneName )
{
	int32 BoneIndex = GetBoneIndex(BoneName);
	if ( BoneIndex != INDEX_NONE )
	{
		UnHideBone(BoneIndex);
	}
}

bool USkinnedMeshComponent::UpdateLODStatus()
{
	// Predict the best (min) LOD level we are going to need. Basically we use the Min (best) LOD the renderer desired last frame.
	// Because we update bones based on this LOD level, we have to update bones to this LOD before we can allow rendering at it.

	if(SkeletalMesh != NULL)
	{
		int32 MaxLODIndex = 0;
		if (MeshObject)
		{
			MaxLODIndex = MeshObject->GetSkeletalMeshResource().LODModels.Num() - 1;
		}

		// Support forcing to a particular LOD.
		if(ForcedLodModel > 0)
		{
			PredictedLODLevel = FMath::Clamp(ForcedLodModel - 1, 0, MaxLODIndex);
		}
		else
		{
			// If no MeshObject - just assume lowest LOD.
			if(MeshObject)
			{
				PredictedLODLevel = FMath::Clamp(MeshObject->MinDesiredLODLevel + GSkeletalMeshLODBias, 0, MaxLODIndex);
			}
			else
			{
				PredictedLODLevel = MaxLODIndex;
			}
		}

		// now check to see if we have a MinLODLevel
		if( ( MinLodModel > 0 ) && ( MinLodModel <= MaxLODIndex ) )
		{
			PredictedLODLevel = FMath::Clamp(PredictedLODLevel, MinLodModel, MaxLODIndex);
		}
	}
	else
	{
		PredictedLODLevel = 0;
	}

	// See if LOD has changed. 
	bool bLODChanged = (PredictedLODLevel != OldPredictedLODLevel);
	OldPredictedLODLevel = PredictedLODLevel;

	// Read back MaxDistanceFactor from the render object.
	if(MeshObject)
	{
		MaxDistanceFactor = MeshObject->MaxDistanceFactor;

#if CHART_DISTANCE_FACTORS
		// Only chart DistanceFactor if it was actually rendered recently
		if(bChartDistanceFactor && ((LastRenderTime > GetWorld()->TimeSeconds - 1.0f) || bUpdateSkelWhenNotRendered))
		{
			AddDistanceFactorToChart(MaxDistanceFactor);
		}
#endif // CHART_DISTANCE_FACTORS
	}

	return bLODChanged;
}

/** See if an array of ActiveVertexAnims already contains the supplied anim */
int32 FindVertexAnim(const TArray<FActiveVertexAnim>& ActiveAnims, UVertexAnimBase* Anim)
{
	for(int32 i=0; i<ActiveAnims.Num(); i++)
	{
		if(ActiveAnims[i].VertAnim == Anim)
		{
			return i;
		}
	}

	return INDEX_NONE;
}

void USkinnedMeshComponent::UpdateActiveVertexAnims(const TMap<FName, float>& MorphCurveAnims, const TArray<FActiveVertexAnim>& ActiveAnims)
{
	// Empty the existing ActiveVertexAnims array
	ActiveVertexAnims.Empty();

	// First copy ActiveAnims
	for(int32 AnimIdx=0; AnimIdx < ActiveAnims.Num(); AnimIdx++)
	{
		const FActiveVertexAnim& ActiveAnim = ActiveAnims[AnimIdx];
		// Check it has valid weight, and works on this SkeletalMesh
		if(	ActiveAnim.Weight > MinVertexAnimBlendWeight &&
			ActiveAnim.VertAnim != NULL &&
			ActiveAnim.VertAnim->BaseSkelMesh == SkeletalMesh )
		{
			ActiveVertexAnims.Add(ActiveAnim);
		}
		// @TODO Need to check for duplicates here?
	}

	// Then go over the CurveKeys finding morph targets by name
	for(auto CurveIter=MorphCurveAnims.CreateConstIterator(); CurveIter; ++CurveIter)
	{
		const FName & CurveName	= (CurveIter).Key();
		const float & Weight	= (CurveIter).Value();

		// If it has a valid weight
		if(Weight > MinVertexAnimBlendWeight)
		{
			// Find morph reference
			UMorphTarget* Target = FindMorphTarget(CurveName);
			if(Target != NULL)				
			{
				// See if this morph target already has an entry
				int32 AnimIndex = FindVertexAnim(ActiveVertexAnims, Target);
				// If not, add it
				if(AnimIndex == INDEX_NONE)
				{
					ActiveVertexAnims.Add(FActiveVertexAnim(Target, Weight));
				}
				// If it does, use the max weight
				else
				{
					const float CurrentWeight = ActiveVertexAnims[AnimIndex].Weight;
					ActiveVertexAnims[AnimIndex].Weight = FMath::Max<float>(CurrentWeight, Weight);
				}
			}
		}
	}
}

