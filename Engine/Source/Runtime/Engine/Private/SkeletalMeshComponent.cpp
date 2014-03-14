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
#include "AnimEncoding.h"
#include "AnimationUtils.h"
#include "AnimationRuntime.h"
#include "PhysXASync.h"

#ifndef EXPERIMENTAL_PARALLEL_CODE  
	#error EXPERIMENTAL_PARALLEL_CODE must be defined as either zero or one
#endif

USkeletalMeshComponent::USkeletalMeshComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bAutoActivate = true;
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	PostPhysicsComponentTick.bCanEverTick = true;
	bWantsInitializeComponent = true;
	GlobalAnimRateScale = 1.0f;
	bNoSkeletonUpdate = false;
	bTickAnimationWhenNotRendered_DEPRECATED = true;
	MeshComponentUpdateFlag = EMeshComponentUpdateFlag::AlwaysTickPoseAndRefreshBones;
	KinematicBonesUpdateType = EKinematicBonesUpdateToPhysics::SkipSimulatingBones;
	bGenerateOverlapEvents = false;
	LineCheckBoundsScale = FVector(1.0f, 1.0f, 1.0f);
#if WITH_APEX_CLOTHING
	ClothMaxDistanceScale = 1.0f;

#if WITH_CLOTH_COLLISION_DETECTION
	ClothingCollisionRevision = 0;
#endif// #if WITH_CLOTH_COLLISION_DETECTION

#endif//#if WITH_APEX_CLOTHING

	DefaultPlayRate_DEPRECATED = 1.0f;
	bDefaultPlaying_DEPRECATED = true;
	bEnablePhysicsOnDedicatedServer = false;
	bEnableUpdateRateOptimizations = false;

#if EXPERIMENTAL_PARALLEL_CODE
	PrimaryComponentTick.TickGroup = TG_ParallelAnimWork;
	PrimaryComponentTick.bRunOnAnyThread = true;	
#endif
}

bool USkeletalMeshComponent::NeedToSpawnAnimScriptInstance(bool bForceInit) const
{
	if (AnimationMode == EAnimationMode::AnimationBlueprint && (AnimBlueprintGeneratedClass != NULL) && 
		(SkeletalMesh != NULL) && (SkeletalMesh->Skeleton->IsCompatible(AnimBlueprintGeneratedClass->TargetSkeleton)))
	{
		if (bForceInit || (AnimScriptInstance == NULL) || (AnimScriptInstance->GetClass() != AnimBlueprintGeneratedClass) )
		{
			return true;
		}
	}

	return false;
}

bool USkeletalMeshComponent::IsAnimBlueprintInstanced() const
{
	return (AnimScriptInstance && AnimScriptInstance->GetClass() == AnimBlueprintGeneratedClass);
}

void USkeletalMeshComponent::OnRegister()
{
	Super::OnRegister();

	InitAnim(false);

	if (MeshComponentUpdateFlag == EMeshComponentUpdateFlag::OnlyTickPoseWhenRendered && !FApp::CanEverRender())
	{
		SetComponentTickEnabled(false);
	}
}

void USkeletalMeshComponent::OnUnregister()
{
#if WITH_APEX_CLOTHING
	//clothing actors will be re-created in TickClothing
	RemoveAllClothingActors();
#endif// #if WITH_APEX_CLOTHING

	Super::OnUnregister();
}

void USkeletalMeshComponent::InitAnim(bool bForceReinit)
{
	// a lot of places just call InitAnim without checking Mesh, so 
	// I'm moving the check here
	if ( SkeletalMesh != NULL && IsRegistered() )
	{
		bool bBlueprintMismatch = (AnimBlueprintGeneratedClass != NULL) && 
			(AnimScriptInstance != NULL) && (AnimScriptInstance->GetClass() != AnimBlueprintGeneratedClass);

		bool bSkeletonMismatch = AnimScriptInstance && AnimScriptInstance->CurrentSkeleton && (AnimScriptInstance->CurrentSkeleton!=SkeletalMesh->Skeleton);

		if (bBlueprintMismatch || bSkeletonMismatch )
		{
			ClearAnimScriptInstance();
		}

		// this has to be called before Initialize Animation because it will required RequiredBones list when InitializeAnimScript
		RecalcRequiredBones(0);

		InitializeAnimScriptInstance(bForceReinit);

		TickAnimation(0.f);
		RefreshBoneTransforms();
		UpdateComponentToWorld();
	}
}

void USkeletalMeshComponent::InitializeAnimScriptInstance(bool bForceReinit)
{
	if (IsRegistered())
	{
		if (NeedToSpawnAnimScriptInstance(bForceReinit))
		{
			AnimScriptInstance = NewObject<UAnimInstance>(this, AnimBlueprintGeneratedClass);
		}
		else if (AnimationMode == EAnimationMode::AnimationSingleNode)
		{
			AnimScriptInstance = NewObject<UAnimSingleNodeInstance>(this);
		}

		if( AnimScriptInstance )
		{
			AnimScriptInstance->InitializeAnimation();
		}
	}
}

bool USkeletalMeshComponent::IsWindEnabled() const
{
#if WITH_APEX_CLOTHING
	// Wind is enabled in game worlds
	return GetWorld() && GetWorld()->IsGameWorld();
#else
	return false;
#endif
}

void USkeletalMeshComponent::ClearAnimScriptInstance()
{
	AnimScriptInstance = NULL;
}

void USkeletalMeshComponent::CreateRenderState_Concurrent()
{
	// Update bHasValidBodies flag
	UpdateHasValidBodies();

#if WITH_APEX_CLOTHING
	//clothing actors will be re-created in TickClothing
	RemoveAllClothingActors();
#endif// #if WITH_APEX_CLOTHING

	Super::CreateRenderState_Concurrent();
}

void USkeletalMeshComponent::InitializeComponent()
{
	Super::InitializeComponent();

	InitAnim(false);

#if EXPERIMENTAL_PARALLEL_CODE
	PrimaryComponentTick.TickGroup = TG_ParallelAnimWork;
	PrimaryComponentTick.bRunOnAnyThread = true;	
#endif

}

#if CHART_DISTANCE_FACTORS
static void AddDistanceFactorToChart(float DistanceFactor)
{
	if(DistanceFactor < SMALL_NUMBER)
	{
		return;
	}

	if(DistanceFactor >= GDistanceFactorDivision[NUM_DISTANCEFACTOR_BUCKETS-2])
	{
		GDistanceFactorChart[NUM_DISTANCEFACTOR_BUCKETS-1]++;
	}
	else if(DistanceFactor < GDistanceFactorDivision[0])
	{
		GDistanceFactorChart[0]++;
	}
	else
	{
		for(int32 i=1; i<NUM_DISTANCEFACTOR_BUCKETS-2; i++)
		{
			if(DistanceFactor < GDistanceFactorDivision[i])
			{
				GDistanceFactorChart[i]++;
				break;
			}
		}
	}
}
#endif // CHART_DISTANCE_FACTORS


void USkeletalMeshComponent::PostPhysicsTick(FPrimitiveComponentPostPhysicsTickFunction &ThisTickFunction)
{
	Super::PostPhysicsTick(ThisTickFunction);

	// if physics is disabled on dedicated server, no reason to be here. 
	if ( !bEnablePhysicsOnDedicatedServer && IsRunningDedicatedServer() )
	{
		return;
	}

	if (IsRegistered() && IsSimulatingPhysics())
	{
		SyncComponentToRBPhysics();
	}

	// this used to not run if not rendered, but that causes issues such as bounds not updated
	// causing it to not rendered, at the end, I think we should blend body positions
	// for example if you're only simulating, this has to happen all the time
	// whether looking at it or not, otherwise
	// @todo better solution is to check if it has moved by changing SyncComponentToRBPhysics to return true if anything modified
	// and run this if that is true or rendered
	// that will at least reduce the chance of mismatch
	// generally if you move your actor position, this has to happen to approximately match their bounds
	if( Bodies.Num() > 0 && IsRegistered() )
	{
		BlendInPhysics();
	}
}

#if WITH_EDITOR
void USkeletalMeshComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	UProperty* PropertyThatChanged = PropertyChangedEvent.Property;

	if ( PropertyThatChanged != NULL )
	{
		// if the blueprint has changed, recreate the AnimInstance
		if ( PropertyThatChanged->GetFName() == GET_MEMBER_NAME_CHECKED( USkeletalMeshComponent, AnimationMode ) )
		{
			if (AnimationMode == EAnimationMode::AnimationBlueprint)
			{
				if (AnimBlueprintGeneratedClass == NULL)
				{
					ClearAnimScriptInstance();
				}
				else
				{
					if (NeedToSpawnAnimScriptInstance(false))
					{
						AnimScriptInstance = NewObject<UAnimInstance>(this, AnimBlueprintGeneratedClass);
						AnimScriptInstance->InitializeAnimation();
					}
				}
			}
		}

		if ( PropertyThatChanged->GetFName() == GET_MEMBER_NAME_CHECKED( USkeletalMeshComponent, AnimBlueprintGeneratedClass ) )
		{
			if (AnimBlueprintGeneratedClass == NULL)
			{
				if (AnimationMode == EAnimationMode::AnimationBlueprint)
				{
					ClearAnimScriptInstance();
				}
			}
			else
			{
				if (NeedToSpawnAnimScriptInstance(false))
				{
					AnimScriptInstance = NewObject<UAnimInstance>(this, AnimBlueprintGeneratedClass);
					AnimScriptInstance->InitializeAnimation();
				}
			}
		}

		if(PropertyThatChanged->GetFName() == GET_MEMBER_NAME_CHECKED( USkeletalMeshComponent, SkeletalMesh))
		{
			ValidateAnimation();

			if(OnSkeletalMeshPropertyChanged.IsBound())
			{
				OnSkeletalMeshPropertyChanged.Broadcast();
			}
		}

		// when user changes simulate physics, just make sure to update blendphysics together
		// bBlendPhysics isn't the editor exposed property, it should work with simulate physics
		if ( PropertyThatChanged->GetFName() == GET_MEMBER_NAME_CHECKED( FBodyInstance, bSimulatePhysics ))
		{
			bBlendPhysics = BodyInstance.bSimulatePhysics;
		}

		if ( PropertyThatChanged->GetFName() == GET_MEMBER_NAME_CHECKED( FSingleAnimationPlayData, AnimToPlay ))
		{
			// make sure the animation skeleton matches the current skeletalmesh
			if (AnimationData.AnimToPlay != NULL && SkeletalMesh && AnimationData.AnimToPlay->GetSkeleton() != SkeletalMesh->Skeleton)
			{
				UE_LOG(LogAnimation, Warning, TEXT("Invalid animation"));
				AnimationData.AnimToPlay = NULL;
			}
		}

		if ( PropertyThatChanged->GetFName() == GET_MEMBER_NAME_CHECKED( FSingleAnimationPlayData, SavedPosition ))
		{
			AnimationData.ValidatePosition();
		}
	}
}

void USkeletalMeshComponent::LoadedFromAnotherClass(const FName& OldClassName)
{
	Super::LoadedFromAnotherClass(OldClassName);

	if(GetLinkerUE4Version() < VER_UE4_REMOVE_SINGLENODEINSTANCE)
	{
		static FName SingleAnimSkeletalComponent_NAME(TEXT("SingleAnimSkeletalComponent"));

		if(OldClassName == SingleAnimSkeletalComponent_NAME)
		{
			SetAnimationMode(EAnimationMode::Type::AnimationSingleNode);

			// support old compatibility code that changed variable name
			if (SequenceToPlay_DEPRECATED!=NULL && AnimToPlay_DEPRECATED== NULL)
			{
				AnimToPlay_DEPRECATED = SequenceToPlay_DEPRECATED;
				SequenceToPlay_DEPRECATED = NULL;
			}

			AnimationData.AnimToPlay = AnimToPlay_DEPRECATED;
			AnimationData.bSavedLooping = bDefaultLooping_DEPRECATED;
			AnimationData.bSavedPlaying = bDefaultPlaying_DEPRECATED;
			AnimationData.SavedPosition = DefaultPosition_DEPRECATED;
			AnimationData.SavedPlayRate = DefaultPlayRate_DEPRECATED;

			MarkPackageDirty();
		}
	}
}
#endif // WITH_EDITOR

void USkeletalMeshComponent::TickAnimation(float DeltaTime)
{
	SCOPE_CYCLE_COUNTER(STAT_AnimTickTime);
	if (SkeletalMesh != NULL)
	{
		if (AnimScriptInstance != NULL)
		{
			// Tick the animation
			AnimScriptInstance->UpdateAnimation(DeltaTime * GlobalAnimRateScale);

			// TODO @LinaH - I've hit access violations due to AnimScriptInstance being NULL after this, probably due to
			// AnimNotifies?  Please take a look and fix as we discussed.  Temporary fix:
			if (AnimScriptInstance != NULL)
			{
				// now all tick/trigger/kismet is done
				// add MorphTarget Curves from Kismet driven or any other source
				// and overwrite if it exists
				// Tick always should maintain this list, not Evaluate
				for( auto Iter = MorphTargetCurves.CreateConstIterator(); Iter; ++Iter )
				{
					float *CurveValPtr = AnimScriptInstance->MorphTargetCurves.Find(Iter.Key());
					if ( CurveValPtr )
					{
						// override the value if Kismet request was made
						*CurveValPtr = Iter.Value();
					}
					else
					{
						AnimScriptInstance->MorphTargetCurves.Add(Iter.Key(), Iter.Value());
					}				
				}

				//Update material parameters
				if(AnimScriptInstance->MaterialParameterCurves.Num() > 0)
				{
					for( auto Iter = AnimScriptInstance->MaterialParameterCurves.CreateConstIterator(); Iter; ++Iter )
					{
						FName ParameterName = Iter.Key();
						float ParameterValue = Iter.Value();

						for(int32 MaterialIndex = 0; MaterialIndex < GetNumMaterials(); ++MaterialIndex)
						{
							UMaterialInterface* MaterialInterface = GetMaterial(MaterialIndex);
							if (MaterialInterface)
							{
								float TestValue; //not used but needed for GetScalarParameterValue call
								if(MaterialInterface->GetScalarParameterValue(ParameterName,TestValue))
								{
									UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(MaterialInterface);
									if(!DynamicMaterial) //Is it already a UMaterialInstanceDynamic (ie we used it last tick)
									{
										DynamicMaterial = CreateAndSetMaterialInstanceDynamic(MaterialIndex);
									}
									DynamicMaterial->SetScalarParameterValue(ParameterName, ParameterValue);
								
									//Assume that we only set the parameter on one of the materials, remove this break
									//if that is no longer desired
									break;
								}
							}
						}
					}
				}
			}
		}
	}
}

bool USkeletalMeshComponent::UpdateLODStatus()
{
	if ( Super::UpdateLODStatus() )
	{
		bRequiredBonesUpToDate = false;

#if WITH_APEX_CLOTHING
		SetClothingLOD(PredictedLODLevel);
#endif // #if WITH_APEX_CLOTHING
		return true;
	}

	return false;
}

bool USkeletalMeshComponent::ShouldUpdateTransform(bool bLODHasChanged) const
{
	// If forcing RefPose we can skip updating the skeleton for perf, except if it's using MorphTargets.
	const bool bSkipBecauseOfRefPose = bForceRefpose && bOldForceRefPose && (MorphTargetCurves.Num() == 0);

	// LOD changing should always trigger an update.
	return (bLODHasChanged || (!bNoSkeletonUpdate && !bSkipBecauseOfRefPose && Super::ShouldUpdateTransform(bLODHasChanged)));
}

bool USkeletalMeshComponent::ShouldTickPose() const
{
	// Characters playing Root Motion will tick pose themselves before physics.
	ACharacter * CharacterOwner = Cast<ACharacter>(GetOwner());
	const bool bSkipBecauseOfRootMotion = CharacterOwner && CharacterOwner->IsPlayingRootMotion();

	// When we stop root motion we go back to ticking after CharacterMovement. Unfortunately that means that we could tick twice that frame.
	// So only enforce a single tick per frame.
	const bool bAlreadyTickedThisFrame = (LastTickTime == GetWorld()->TimeSeconds);

	// Remote Clients on the Server will always tick animations as updates from the client comes in. To use Client's delta time.
	const bool bRemoteClientOnServer = CharacterOwner && (CharacterOwner->Role == ROLE_Authority) && CharacterOwner->Controller && !CharacterOwner->Controller->IsLocalController();

	return (Super::ShouldTickPose() && IsRegistered() && AnimScriptInstance && !bPauseAnims && GetWorld()->HasBegunPlay() && !bNoSkeletonUpdate && !bSkipBecauseOfRootMotion && !bRemoteClientOnServer && !bAlreadyTickedThisFrame);
}

void USkeletalMeshComponent::TickPose(float DeltaTime)
{
	TickAnimation( DeltaTime );
	LastTickTime = GetWorld()->TimeSeconds;
}

void USkeletalMeshComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
#if EXPERIMENTAL_PARALLEL_CODE
	if (ThisTickFunction->TickGroup != TG_ParallelAnimWork)
	{			
		UE_LOG(LogAnimation, Warning, TEXT("USkeletalMeshComponent %s is not in TickGroup TG_ParallelAnimWork! Group=%d, GoWide=%d"), *(GetOwner()->GetName()), (int32)ThisTickFunction->TickGroup, ThisTickFunction->bRunOnAnyThread);
	}
#endif

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Update bOldForceRefPose
	bOldForceRefPose = bForceRefpose;

	TickClothing();
}


/** 
 *	Utility for taking two arrays of bone indices, which must be strictly increasing, and finding the intersection between them.
 *	That is - any item in the output should be present in both A and B. Output is strictly increasing as well.
 */
static void IntersectBoneIndexArrays(TArray<FBoneIndexType>& Output, const TArray<FBoneIndexType>& A, const TArray<FBoneIndexType>& B)
{
	int32 APos = 0;
	int32 BPos = 0;
	while(	APos < A.Num() && BPos < B.Num() )
	{
		// If value at APos is lower, increment APos.
		if( A[APos] < B[BPos] )
		{
			APos++;
		}
		// If value at BPos is lower, increment APos.
		else if( B[BPos] < A[APos] )
		{
			BPos++;
		}
		// If they are the same, put value into output, and increment both.
		else
		{
			Output.Add( A[APos] );
			APos++;
			BPos++;
		}
	}
}


void USkeletalMeshComponent::FillSpaceBases()
{
	SCOPE_CYCLE_COUNTER(STAT_SkelComposeTime);

	if( !SkeletalMesh )
	{
		return;
	}

	// right now all this does is to convert to SpaceBases
	check( SkeletalMesh->RefSkeleton.GetNum() == LocalAtoms.Num() );
	check( SkeletalMesh->RefSkeleton.GetNum() == SpaceBases.Num() );
	check( SkeletalMesh->RefSkeleton.GetNum() == BoneVisibilityStates.Num() );

	const int32 NumBones = LocalAtoms.Num();

#if (UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT)
	/** Keep track of which bones have been processed for fast look up */
	TArray<uint8> BoneProcessed;
	BoneProcessed.AddZeroed(NumBones);
#endif

	FTransform * LocalTransformsData = LocalAtoms.GetTypedData(); 
	FTransform * SpaceBasesData = SpaceBases.GetTypedData();

	// First bone is always root bone, and it doesn't have a parent.
	{
		check( RequiredBones[0] == 0 );
		SpaceBases[0] = LocalAtoms[0];

#if (UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT)
		// Mark bone as processed
		BoneProcessed[0] = 1;
#endif
	}

	for(int32 i=1; i<RequiredBones.Num(); i++)
	{
		const int32 BoneIndex = RequiredBones[i];
		FPlatformMisc::Prefetch(SpaceBasesData + BoneIndex);

#if (UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT)
		// Mark bone as processed
		BoneProcessed[BoneIndex] = 1;
#endif
		// For all bones below the root, final component-space transform is relative transform * component-space transform of parent.
		const int32 ParentIndex = SkeletalMesh->RefSkeleton.GetParentIndex(BoneIndex);
		FPlatformMisc::Prefetch(SpaceBasesData + ParentIndex);

#if (UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT)
		// Check the precondition that Parents occur before Children in the RequiredBones array.
		checkSlow(BoneProcessed[ParentIndex] == 1);
#endif
		FTransform::Multiply(SpaceBasesData + BoneIndex, LocalTransformsData + BoneIndex, SpaceBasesData + ParentIndex);

		checkSlow( SpaceBases[BoneIndex].IsRotationNormalized() );
		checkSlow( !SpaceBases[BoneIndex].ContainsNaN() );
	}
}

/** Takes sorted array Base and then adds any elements from sorted array Insert which is missing from it, preserving order. */
static void MergeInBoneIndexArrays(TArray<FBoneIndexType>& BaseArray, TArray<FBoneIndexType>& InsertArray)
{
	// Then we merge them into the array of required bones.
	int32 BaseBonePos = 0;
	int32 InsertBonePos = 0;

	// Iterate over each of the bones we need.
	while( InsertBonePos < InsertArray.Num() )
	{
		// Find index of physics bone
		FBoneIndexType InsertBoneIndex = InsertArray[InsertBonePos];

		// If at end of BaseArray array - just append.
		if( BaseBonePos == BaseArray.Num() )
		{
			BaseArray.Add(InsertBoneIndex);
			BaseBonePos++;
			InsertBonePos++;
		}
		// If in the middle of BaseArray, merge together.
		else
		{
			// Check that the BaseArray array is strictly increasing, otherwise merge code does not work.
			check( BaseBonePos == 0 || BaseArray[BaseBonePos-1] < BaseArray[BaseBonePos] );

			// Get next required bone index.
			FBoneIndexType BaseBoneIndex = BaseArray[BaseBonePos];

			// We have a bone in BaseArray not required by Insert. Thats ok - skip.
			if( BaseBoneIndex < InsertBoneIndex )
			{
				BaseBonePos++;
			}
			// Bone required by Insert is in 
			else if( BaseBoneIndex == InsertBoneIndex )
			{
				BaseBonePos++;
				InsertBonePos++;
			}
			// Bone required by Insert is missing - insert it now.
			else // BaseBoneIndex > InsertBoneIndex
			{
				BaseArray.InsertUninitialized(BaseBonePos);
				BaseArray[BaseBonePos] = InsertBoneIndex;

				BaseBonePos++;
				InsertBonePos++;
			}
		}
	}
}


void USkeletalMeshComponent::RecalcRequiredBones(int32 LODIndex)
{
	if (!SkeletalMesh)
	{
		return;
	}

	FSkeletalMeshResource* SkelMeshResource = GetSkeletalMeshResource();
	check(SkelMeshResource);

	// The list of bones we want is taken from the predicted LOD level.
	FStaticLODModel& LODModel = SkelMeshResource->LODModels[LODIndex];
	RequiredBones = LODModel.RequiredBones;
	
	const UPhysicsAsset* const PhysicsAsset = GetPhysicsAsset();
	// If we have a PhysicsAsset, we also need to make sure that all the bones used by it are always updated, as its used
	// by line checks etc. We might also want to kick in the physics, which means having valid bone transforms.
	if(PhysicsAsset)
	{
		TArray<FBoneIndexType> PhysAssetBones;
		for(int32 i=0; i<PhysicsAsset->BodySetup.Num(); i++ )
		{
			int32 PhysBoneIndex = SkeletalMesh->RefSkeleton.FindBoneIndex( PhysicsAsset->BodySetup[i]->BoneName );
			if(PhysBoneIndex != INDEX_NONE)
			{
				PhysAssetBones.Add(PhysBoneIndex);
			}	
		}

		// Then sort array of required bones in hierarchy order
		PhysAssetBones.Sort();

		// Make sure all of these are in RequiredBones.
		MergeInBoneIndexArrays(RequiredBones, PhysAssetBones);
	}

	// Make sure that bones with per-poly collision are also always updated.
	// TODO UE4

	// Purge invisible bones and their children
	// this has to be done before mirror table check/phsysics body checks
	// mirror table/phys body ones has to be calculated
	if (ShouldUpdateBoneVisibility())
	{
		check(BoneVisibilityStates.Num() == SpaceBases.Num());

		int32 VisibleBoneWriteIndex = 0;
		for (int32 i = 0; i < RequiredBones.Num(); ++i)
		{
			FBoneIndexType CurBoneIndex = RequiredBones[i];

			// Current bone visible?
			if (BoneVisibilityStates[CurBoneIndex] == BVS_Visible)
			{
				RequiredBones[VisibleBoneWriteIndex++] = CurBoneIndex;
			}
		}

		// Remove any trailing junk in the RequiredBones array
		const int32 NumBonesHidden = RequiredBones.Num() - VisibleBoneWriteIndex;
		if (NumBonesHidden > 0)
		{
			RequiredBones.RemoveAt(VisibleBoneWriteIndex, NumBonesHidden);
		}
	}

	// Add in any bones that may be required when mirroring.
	// JTODO: This is only required if there are mirroring nodes in the tree, but hard to know...
	if(SkeletalMesh->SkelMirrorTable.Num() > 0 && 
		SkeletalMesh->SkelMirrorTable.Num() == LocalAtoms.Num())
	{
		TArray<FBoneIndexType> MirroredDesiredBones;
		MirroredDesiredBones.AddUninitialized(RequiredBones.Num());

		// Look up each bone in the mirroring table.
		for(int32 i=0; i<RequiredBones.Num(); i++)
		{
			MirroredDesiredBones[i] = SkeletalMesh->SkelMirrorTable[RequiredBones[i]].SourceIndex;
		}

		// Sort to ensure strictly increasing order.
		MirroredDesiredBones.Sort();

		// Make sure all of these are in RequiredBones, and 
		MergeInBoneIndexArrays(RequiredBones, MirroredDesiredBones);
	}

	// Ensure that we have a complete hierarchy down to those bones.
	FAnimationRuntime::EnsureParentsPresent(RequiredBones, SkeletalMesh);

	// make sure animation requiredBone to mark as dirty
	if (AnimScriptInstance)
	{
		AnimScriptInstance->RecalcRequiredBones();
	}

	bRequiredBonesUpToDate = true;

	// Invalidate cached bones.
	CachedLocalAtoms.Empty();
	CachedSpaceBases.Empty();
}

void USkeletalMeshComponent::EvaluateAnimation(/*FTransform & ExtractedRootMotionDelta, int32 & bHasRootMotion*/)
{
	SCOPE_CYCLE_COUNTER(STAT_AnimBlendTime);

	if( !SkeletalMesh )
	{
		return;
	}

	// We can only evaluate animation if RequiredBones is properly setup for the right mesh!
	if( SkeletalMesh->Skeleton && AnimScriptInstance 
		&& ensure(bRequiredBonesUpToDate)
		&& AnimScriptInstance->RequiredBones.IsValid() 
		&& (AnimScriptInstance->RequiredBones.GetAsset() == SkeletalMesh) )
	{
		if( !bForceRefpose )
		{
			// Create an evaluation context
			FPoseContext EvaluationContext(AnimScriptInstance);
			EvaluationContext.ResetToRefPose();
			
			// Run the anim blueprint
			AnimScriptInstance->EvaluateAnimation(EvaluationContext);

			// can we avoid that copy?
			if( EvaluationContext.Pose.Bones.Num() > 0 )
			{
				LocalAtoms = EvaluationContext.Pose.Bones;
			}
			else
			{
				FAnimationRuntime::FillWithRefPose(LocalAtoms, AnimScriptInstance->RequiredBones);
			}
		}
		else
		{
			FAnimationRuntime::FillWithRefPose(LocalAtoms, AnimScriptInstance->RequiredBones);
		}

		UpdateActiveVertexAnims(AnimScriptInstance->MorphTargetCurves, AnimScriptInstance->VertexAnims);
	}
	else
	{
		LocalAtoms = SkeletalMesh->RefSkeleton.GetRefBonePose();

		// if it's only morph, there is no reason to blend
		if ( MorphTargetCurves.Num() > 0 )
		{
			TArray<struct FActiveVertexAnim> EmptyVertexAnims;
			UpdateActiveVertexAnims(MorphTargetCurves, EmptyVertexAnims);
		}
	}

	// Remember the root bone's translation so we can move the bounds.
	RootBoneTranslation = LocalAtoms[0].GetTranslation() - SkeletalMesh->RefSkeleton.GetRefBonePose()[0].GetTranslation();
}

void USkeletalMeshComponent::UpdateSlaveComponent()
{
	check (MasterPoseComponent);

	if (MasterPoseComponent->IsA(USkeletalMeshComponent::StaticClass()))
	{
		USkeletalMeshComponent* MasterSMC= CastChecked<USkeletalMeshComponent>(MasterPoseComponent);

		if ( MasterSMC->AnimScriptInstance )
		{
			UpdateActiveVertexAnims(MasterSMC->AnimScriptInstance->MorphTargetCurves, MasterSMC->AnimScriptInstance->VertexAnims);
		}
	}

	Super::UpdateSlaveComponent();
}

void USkeletalMeshComponent::RefreshBoneTransforms()
{
	SCOPE_CYCLE_COUNTER(STAT_RefreshBoneTransforms);

	// Can't do anything without a SkeletalMesh
	// Do nothing more if no bones in skeleton.
	if( !SkeletalMesh || SpaceBases.Num() == 0 )
	{
		return;
	}
	
	AActor * Owner = GetOwner();
	const FAnimUpdateRateParameters  & UpdateRateParams = Owner ? Owner->AnimUpdateRateParams : FAnimUpdateRateParameters();

	{
		FScopeLockPhysXWriter LockPhysXForWriting;

		// Recalculate the RequiredBones array, if necessary
		if( !bRequiredBonesUpToDate )
		{
			RecalcRequiredBones(PredictedLODLevel);
		}

		// Update rate turned off, evaluate every frame.
		if( !bEnableUpdateRateOptimizations || (UpdateRateParams.GetEvaluationRate() <= 1) )
		{
			// evaluate pure animations, and fill up LocalAtoms
			EvaluateAnimation();
			// We need the mesh space bone transforms now for renderer to get delta from ref pose:
			FillSpaceBases();
			// Invalidate cached bones.
			CachedLocalAtoms.Empty();
			CachedSpaceBases.Empty();
		}
		else
		{
			// figure out if our cache is invalid.
			const bool bInvalidCachedBones = (LocalAtoms.Num() != SkeletalMesh->RefSkeleton.GetNum()) 
				|| (LocalAtoms.Num() != CachedLocalAtoms.Num())
				|| (SpaceBases.Num() != CachedSpaceBases.Num());

			// If cache is invalid, we need to rebuild it. And we can't interpolate.
			// (same path if we're not interpolating and not skipping a frame).
			if( bInvalidCachedBones || (!UpdateRateParams.ShouldInterpolateSkippedFrames() && !UpdateRateParams.ShouldSkipEvaluation()) )
			{
				// evaluate pure animations, and fill up LocalAtoms
				EvaluateAnimation();
				// Fill SpaceBases from LocalAtoms
				FillSpaceBases();

				// Cache bones
				CachedLocalAtoms = LocalAtoms;
				CachedSpaceBases = SpaceBases;
			}
			else
			{
				// No interpolation, just copy
				// @todo: if we don't blend any physics, we could even skip the copy.
				if( !UpdateRateParams.ShouldInterpolateSkippedFrames() )
				{
					LocalAtoms = CachedLocalAtoms;
					SpaceBases = CachedSpaceBases;
				}
				else
				{
					// If we are not skipping evaluation this frame, refresh cache.
					if( !UpdateRateParams.ShouldSkipEvaluation() )
					{
						// Preserve LocalAtoms and SpaceBases, so we can keep interpolation.
						Exchange(LocalAtoms, CachedLocalAtoms);
						Exchange(SpaceBases, CachedSpaceBases);

						// evaluate pure animations, and fill up LocalAtoms
						EvaluateAnimation();
						// Fill SpaceBases from LocalAtoms
						FillSpaceBases();

						Exchange(LocalAtoms, CachedLocalAtoms);
						Exchange(SpaceBases, CachedSpaceBases);
					}

					// Interpolate
					{
						SCOPE_CYCLE_COUNTER(STAT_InterpolateSkippedFrames);
						const float Alpha = 0.25f + (1.f / float(FMath::Max(UpdateRateParams.GetEvaluationRate(), 2) * 2));
						FAnimationRuntime::LerpBoneTransforms(LocalAtoms, CachedLocalAtoms, Alpha, RequiredBones);
						FAnimationRuntime::LerpBoneTransforms(SpaceBases, CachedSpaceBases, Alpha, RequiredBones);
					}
				}
			}
		}

		// Transforms updated, cached local bounds are now out of date.
		InvalidateCachedBounds();

		// update physics data from animated data
		UpdateKinematicBonesToPhysics(false);
		UpdateRBJointMotors();
	}

	// @todo anim : hack TTP 224385	ANIM: Skeletalmesh double buffer
	// this is problem because intermediate buffer changes physics position as well
	// this causes issue where a half of frame, physics position is fixed with anim pose, and the other half is real simulated position
	// if you enable physics in tick, since that's before physics update, you'll get animation pose dominating physics pose, which isn't what you want. (Or what you'll see)
	// so do not update transform if physics is on. This problem will be solved by double buffer, when we keep one buffer for intermediate, and the other buffer for result query
	{
		FScopeLockPhysXReader LockPhysXForReading;

		if( !IsSimulatingPhysics() )
		{
			SCOPE_CYCLE_COUNTER(STAT_UpdateLocalToWorldAndOverlaps);
			
			// New bone positions need to be sent to render thread
			UpdateComponentToWorld();	

			// animation often change overlap. 
			UpdateOverlaps();
		}
	}

	MarkRenderDynamicDataDirty();
}

//
//	USkeletalMeshComponent::UpdateBounds
//

void USkeletalMeshComponent::UpdateBounds()
{
#if WITH_EDITOR
	FBoxSphereBounds OriginalBounds = Bounds; // Save old bounds
#endif

	// if use parent bound if attach parent exists, and the flag is set
	// since parents tick first before child, this should work correctly
	if ( bUseAttachParentBound && AttachParent != NULL )
	{
		Bounds = AttachParent->Bounds;
	}
	else
	{
		// fixme laurent - extend concept of LocalBounds to all SceneComponent
		// as rendered calls CalcBounds*() directly in FScene::UpdatePrimitiveTransform, which is pretty expensive for SkelMeshes.
		// No need to calculated that again, just use cached local bounds.
		if( bCachedLocalBoundsUpToDate )
		{
			Bounds = CachedLocalBounds.TransformBy(ComponentToWorld);
		}
		else
		{
			// Calculate new bounds
			Bounds = CalcBounds(ComponentToWorld);

			bCachedLocalBoundsUpToDate = true;
			CachedLocalBounds = Bounds.TransformBy(ComponentToWorld.InverseSafe());
		}
	}

#if WITH_EDITOR
	// If bounds have changed (in editor), trigger data rebuild
	if ( IsRegistered() && (World != NULL) && !GetWorld()->IsGameWorld() && 
		(OriginalBounds.Origin.Equals(Bounds.Origin) == false || OriginalBounds.BoxExtent.Equals(Bounds.BoxExtent) == false) )
	{
		GEngine->TriggerStreamingDataRebuild();
	}
#endif
}

FBoxSphereBounds USkeletalMeshComponent::CalcBounds(const FTransform & LocalToWorld) const
{
	FVector RootBoneOffset;

	// if to use MasterPoseComponent's fixed skel bounds, 
	// send MasterPoseComponent's Root Bone Translation
	if( MasterPoseComponent && MasterPoseComponent->SkeletalMesh && 
		MasterPoseComponent->bComponentUseFixedSkelBounds &&
		MasterPoseComponent->IsA((USkeletalMeshComponent::StaticClass())))
	{
		USkeletalMeshComponent* BaseComponent = CastChecked<USkeletalMeshComponent>(MasterPoseComponent);
		RootBoneOffset = BaseComponent->RootBoneTranslation; // Adjust bounds by root bone translation
	}
	else
	{
		RootBoneOffset = RootBoneTranslation;
	}

	FBoxSphereBounds NewBounds = CalcMeshBound( RootBoneOffset, bHasValidBodies, LocalToWorld );
#if WITH_APEX_CLOTHING
	AddClothingBounds(NewBounds);
#endif// #if WITH_APEX_CLOTHING
	return NewBounds;
}

void USkeletalMeshComponent::SetSkeletalMesh(USkeletalMesh* InSkelMesh)
{
	if (InSkelMesh == SkeletalMesh)
	{
		// do nothing if the input mesh is the same mesh we're already using.
		return;
	}

	UPhysicsAsset* OldPhysAsset = GetPhysicsAsset();

	Super::SetSkeletalMesh(InSkelMesh);

#if WITH_EDITOR
	ValidateAnimation();
#endif

	if (GetPhysicsAsset() != OldPhysAsset && IsPhysicsStateCreated())
	{
		RecreatePhysicsState();
	}

	UpdateHasValidBodies();

	InitAnim(false);

#if WITH_APEX_CLOTHING
	ValidateClothingActors();
#endif
}

bool USkeletalMeshComponent::AllocateTransformData()
{
	// Allocate transforms if not present.
	if ( Super::AllocateTransformData() )
	{
		if( LocalAtoms.Num() != SkeletalMesh->RefSkeleton.GetNum() )
		{
			LocalAtoms.Empty( SkeletalMesh->RefSkeleton.GetNum() );
			LocalAtoms.AddUninitialized( SkeletalMesh->RefSkeleton.GetNum() );
		}

		return true;
	}

	LocalAtoms.Empty();
	
	return false;
}

void USkeletalMeshComponent::DeallocateTransformData()
{
	Super::DeallocateTransformData();
	LocalAtoms.Empty();
}

void USkeletalMeshComponent::SetForceRefPose(bool bNewForceRefPose)
{
	bForceRefpose = bNewForceRefPose;
	MarkRenderStateDirty();
}

void USkeletalMeshComponent::SetAnimClass(class UClass* NewClass)
{
	if (NewClass != NULL)
	{
		UAnimBlueprintGeneratedClass* NewGeneratedClass = Cast<UAnimBlueprintGeneratedClass>(NewClass);
		ensure(NULL != NewGeneratedClass);
		// set the animation mode
		AnimationMode = EAnimationMode::Type::AnimationBlueprint;

		if (NewGeneratedClass != AnimBlueprintGeneratedClass)
		{
			// Only need to initialize if it hasn't already been set.
			AnimBlueprintGeneratedClass = NewGeneratedClass;
			ClearAnimScriptInstance();
			InitAnim(true);
		}
	}
	else
	{
		// Need to clear the instance as well as the blueprint.
		// @todo is this it?
		AnimBlueprintGeneratedClass = NULL;
		ClearAnimScriptInstance();
	}
}

UAnimInstance* USkeletalMeshComponent::GetAnimInstance() const
{
	return AnimScriptInstance;
}

FMatrix USkeletalMeshComponent::GetTransformMatrix() const
{
	FTransform RootTransform = GetBoneTransform(0);
	FVector Translation;
	FQuat Rotation;
	
	// if in editor, it should always use localToWorld
	// if root motion is ignored, use root transform 
	if( GetWorld()->IsGameWorld() || !SkeletalMesh )
	{
		// add root translation info
		Translation = RootTransform.GetLocation();
	}
	else
	{
		Translation = ComponentToWorld.TransformPosition(SkeletalMesh->RefSkeleton.GetRefBonePose()[0].GetTranslation());
	}

	// if root rotation is ignored, use root transform rotation
	Rotation = RootTransform.GetRotation();

	// now I need to get scale
	// only LocalToWorld will have scale
	FVector ScaleVector = ComponentToWorld.GetScale3D();

	Rotation.Normalize();
	return FScaleMatrix(ScaleVector)*FQuatRotationTranslationMatrix(Rotation, Translation);
}

void USkeletalMeshComponent::NotifySkelControlBeyondLimit( USkelControlLookAt* LookAt ) {}



void USkeletalMeshComponent::SkelMeshCompOnParticleSystemFinished( UParticleSystemComponent* PSC )
{
	PSC->DetachFromParent();
	PSC->UnregisterComponent();
}


void USkeletalMeshComponent::HideBone( int32 BoneIndex, EPhysBodyOp PhysBodyOption)
{
	Super::HideBone(BoneIndex, PhysBodyOption);

	if (!SkeletalMesh)
	{
		return;
	}

	LocalAtoms[ BoneIndex ].SetScale3D(FVector::ZeroVector);
	bRequiredBonesUpToDate = false;

	if( PhysBodyOption!=PBO_None )
	{
		FName HideBoneName = SkeletalMesh->RefSkeleton.GetBoneName(BoneIndex);
		if ( PhysBodyOption == PBO_Term )
		{
			TermBodiesBelow(HideBoneName);
		}
		else if ( PhysBodyOption == PBO_Disable )
		{
			// Disable collision
			// @JTODO
			//SetCollisionBelow(false, HideBoneName);
		}
	}
}

void USkeletalMeshComponent::UnHideBone( int32 BoneIndex )
{
	Super::UnHideBone(BoneIndex);

	if (!SkeletalMesh)
	{
		return;
	}

	LocalAtoms[ BoneIndex ].SetScale3D(FVector(1.0f));
	bRequiredBonesUpToDate = false;

	FName HideBoneName = SkeletalMesh->RefSkeleton.GetBoneName(BoneIndex);
	// It's okay to turn this on for terminated bodies
	// It won't do any if BodyData isn't found
	// @JTODO
	//SetCollisionBelow(true, HideBoneName);

}


bool USkeletalMeshComponent::IsAnySimulatingPhysics() const
{
	if( bUseSingleBodyPhysics )
	{
		return IsSimulatingPhysics();
	}

	for ( int32 BodyIndex=0; BodyIndex<Bodies.Num(); ++BodyIndex )
	{
		if (Bodies[BodyIndex]->IsInstanceSimulatingPhysics())
		{
			return true;
		}
	}

	return false;
}

/** 
 * Render bones for debug display
 */
void USkeletalMeshComponent::DebugDrawBones(UCanvas* Canvas, bool bSimpleBones) const
{
	if (GetWorld()->IsGameWorld() && SkeletalMesh && Canvas)
	{
		// draw spacebases, we could cache parent bones, but this is mostly debug feature, I'm not caching it right now
		for ( int32 Index=0; Index<RequiredBones.Num(); ++Index )
		{
			int32 BoneIndex = RequiredBones[Index];
			int32 ParentIndex = SkeletalMesh->RefSkeleton.GetParentIndex(BoneIndex);
			FTransform BoneTM = (SpaceBases[BoneIndex] * ComponentToWorld);
			FVector Start, End;
			FLinearColor LineColor;

			End = BoneTM.GetLocation();

			if (ParentIndex >=0)
			{
				Start = (SpaceBases[ParentIndex] * ComponentToWorld).GetLocation();
				LineColor = FLinearColor::White;
			}
			else
			{
				Start = ComponentToWorld.GetLocation();
				LineColor = FLinearColor::Red;
			}

			if(bSimpleBones)
			{
				DrawDebugCanvasLine(Canvas, Start, End, LineColor);
			}
			else
			{
				static const float SphereRadius = 1.0f;

				//Calc cone size 
				FVector EndToStart = (Start-End);
				float ConeLength = EndToStart.Size();
				float Angle = FMath::RadiansToDegrees(FMath::Atan(SphereRadius / ConeLength));

				DrawDebugCanvasWireSphere(Canvas, End, LineColor, SphereRadius, 10);
				DrawDebugCanvasWireCone(Canvas, FTransform(FRotationMatrix::MakeFromX(EndToStart)*FTranslationMatrix(End)), ConeLength, Angle, 4, LineColor);
			}

			RenderAxisGizmo(BoneTM, Canvas);
		}
	}
}

// Render a coordinate system indicator
void USkeletalMeshComponent::RenderAxisGizmo( const FTransform& Transform, UCanvas* Canvas ) const
{
	// Display colored coordinate system axes for this joint.
	const float AxisLength = 3.75f;
	const FVector Origin = Transform.GetLocation();

	// Red = X
	FVector XAxis = Transform.TransformVector( FVector(1.0f,0.0f,0.0f) );
	XAxis.Normalize();
	DrawDebugCanvasLine(Canvas, Origin, Origin + XAxis * AxisLength, FLinearColor( 1.f, 0.3f, 0.3f));		

	// Green = Y
	FVector YAxis = Transform.TransformVector( FVector(0.0f,1.0f,0.0f) );
	YAxis.Normalize();
	DrawDebugCanvasLine(Canvas, Origin, Origin + YAxis * AxisLength, FLinearColor( 0.3f, 1.f, 0.3f));	

	// Blue = Z
	FVector ZAxis = Transform.TransformVector( FVector(0.0f,0.0f,1.0f) );
	ZAxis.Normalize();
	DrawDebugCanvasLine(Canvas, Origin, Origin + ZAxis * AxisLength, FLinearColor( 0.3f, 0.3f, 1.f));	
}

void USkeletalMeshComponent::SetMorphTarget(FName MorphTargetName, float Value)
{
	float *CurveValPtr = MorphTargetCurves.Find(MorphTargetName);
	if ( FPlatformMath::Abs(Value) > ZERO_ANIMWEIGHT_THRESH )
	{
		if ( CurveValPtr )
		{
			// sum up, in the future we might normalize, but for now this just sums up
			// this won't work well if all of them have full weight - i.e. additive 
			*CurveValPtr = Value;
		}
		else
		{
			MorphTargetCurves.Add(MorphTargetName, Value);
		}
	}
	// if less than ZERO_ANIMWEIGHT_THRESH
	// no reason to keep them on the list
	else 
	{
		// remove if found
		MorphTargetCurves.Remove(MorphTargetName);
	}
}

void USkeletalMeshComponent::ClearMorphTargets()
{
	MorphTargetCurves.Empty();
}

float USkeletalMeshComponent::GetMorphTarget( FName MorphTargetName ) const
{
	const float *CurveValPtr = MorphTargetCurves.Find(MorphTargetName);
	
	if(CurveValPtr)
	{
		return *CurveValPtr;
	}
	else
	{
		return 0.0f;
	}
}

FVector USkeletalMeshComponent::GetClosestCollidingRigidBodyLocation(const FVector& TestLocation) const
{
	float BestDistSq = BIG_NUMBER;
	FVector Best = TestLocation;

	UPhysicsAsset* PhysicsAsset = GetPhysicsAsset();
	if( PhysicsAsset )
	{
		for (int32 i=0; i<Bodies.Num(); i++)
		{
			FBodyInstance* BodyInstance = Bodies[i];
			if( BodyInstance && BodyInstance->IsValidBodyInstance() && (BodyInstance->GetCollisionEnabled() != ECollisionEnabled::NoCollision) )
			{
				const FVector BodyLocation = BodyInstance->GetUnrealWorldTransform().GetTranslation();
				const float DistSq = (BodyLocation - TestLocation).SizeSquared();
				if( DistSq < BestDistSq )
				{
					Best = BodyLocation;
					BestDistSq = DistSq;
				}
			}
		}
	}

	return Best;
}

SIZE_T USkeletalMeshComponent::GetResourceSize( EResourceSizeMode::Type Mode )
{
	SIZE_T ResSize = 0;

	for (int32 i=0; i < Bodies.Num(); ++i)
	{
		if (Bodies[i] != NULL && Bodies[i]->IsValidBodyInstance())
		{
			ResSize += Bodies[i]->GetBodyInstanceResourceSize(Mode);
		}
	}

	return ResSize;
}

void USkeletalMeshComponent::SetAnimationMode(EAnimationMode::Type InAnimationMode)
{
	if (AnimationMode != InAnimationMode)
	{
		AnimationMode = InAnimationMode;
		ClearAnimScriptInstance();
		InitializeAnimScriptInstance();
	}
}

EAnimationMode::Type USkeletalMeshComponent::GetAnimationMode() const
{
	return AnimationMode;
}

void USkeletalMeshComponent::PlayAnimation(class UAnimationAsset* NewAnimToPlay, bool bLooping)
{
	SetAnimationMode(EAnimationMode::AnimationSingleNode);
	SetAnimation(NewAnimToPlay);
	Play(bLooping);
}

void USkeletalMeshComponent::SetAnimation(UAnimationAsset* NewAnimToPlay)
{
	UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance();
	if (SingleNodeInstance)
	{
		SingleNodeInstance->SetAnimationAsset(NewAnimToPlay, false);
		SingleNodeInstance->bPlaying = false;
	}
	else if( AnimScriptInstance != NULL )
	{
		UE_LOG(LogAnimation, Warning, TEXT("Currently in Animation Blueprint mode. Please change AnimationMode to Use Animation Asset"));
	}
}

void USkeletalMeshComponent::SetVertexAnimation(UVertexAnimation* NewVertexAnimation)
{
	UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance();
	if (SingleNodeInstance)
	{
		SingleNodeInstance->SetVertexAnimation(NewVertexAnimation, false);
		// when set the asset, we shouldn't automatically play. 
		SingleNodeInstance->bPlaying = false;
	}
	else if( AnimScriptInstance != NULL )
	{
		UE_LOG(LogAnimation, Warning, TEXT("Currently in Animation Blueprint mode. Please change AnimationMode to Use Animation Asset"));
	}
}

void USkeletalMeshComponent::Play(bool bLooping)
{
	UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance();
	if (SingleNodeInstance)
	{
		SingleNodeInstance->bPlaying = true;
		SingleNodeInstance->bLooping = bLooping;
	}
	else if( AnimScriptInstance != NULL )
	{
		UE_LOG(LogAnimation, Warning, TEXT("Currently in Animation Blueprint mode. Please change AnimationMode to Use Animation Asset"));
	}
}

void USkeletalMeshComponent::Stop()
{
	UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance();
	if (SingleNodeInstance)
	{
		SingleNodeInstance->bPlaying = false;
	}
	else if( AnimScriptInstance != NULL )
	{
		UE_LOG(LogAnimation, Warning, TEXT("Currently in Animation Blueprint mode. Please change AnimationMode to Use Animation Asset"));
	}
}

bool USkeletalMeshComponent::IsPlaying() const
{
	UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance();
	if (SingleNodeInstance)
	{
		return SingleNodeInstance->bPlaying;
	}
	else if( AnimScriptInstance != NULL )
	{
		UE_LOG(LogAnimation, Warning, TEXT("Currently in Animation Blueprint mode. Please change AnimationMode to Use Animation Asset"));
	}

	return false;
}

void USkeletalMeshComponent::SetPosition(float InPos)
{
	UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance();
	if (SingleNodeInstance)
	{
		SingleNodeInstance->SetPosition(InPos);
	}
	else if( AnimScriptInstance != NULL )
	{
		UE_LOG(LogAnimation, Warning, TEXT("Currently in Animation Blueprint mode. Please change AnimationMode to Use Animation Asset"));
	}
}

float USkeletalMeshComponent::GetPosition() const
{
	UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance();
	if (SingleNodeInstance)
	{
		return SingleNodeInstance->CurrentTime;
	}
	else if( AnimScriptInstance != NULL )
	{
		UE_LOG(LogAnimation, Warning, TEXT("Currently in Animation Blueprint mode. Please change AnimationMode to Use Animation Asset"));
	}

	return 0.f;
}

void USkeletalMeshComponent::SetPlayRate(float Rate)
{
	UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance();
	if (SingleNodeInstance)
	{
		SingleNodeInstance->SetPlayRate(Rate);
	}
	else if( AnimScriptInstance != NULL )
	{
		UE_LOG(LogAnimation, Warning, TEXT("Currently in Animation Blueprint mode. Please change AnimationMode to Use Animation Asset"));
	}
}

float USkeletalMeshComponent::GetPlayRate() const
{
	UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance();
	if (SingleNodeInstance)
	{
		return SingleNodeInstance->PlayRate;
	}
	else if( AnimScriptInstance != NULL )
	{
		UE_LOG(LogAnimation, Warning, TEXT("Currently in Animation Blueprint mode. Please change AnimationMode to Use Animation Asset"));
	}

	return 0.f;
}

class UAnimSingleNodeInstance* USkeletalMeshComponent::GetSingleNodeInstance() const
{
	return Cast<class UAnimSingleNodeInstance>(AnimScriptInstance);
}

void FSingleAnimationPlayData::Initialize(UAnimSingleNodeInstance* Instance)
{
	Instance->SetAnimationAsset(AnimToPlay);
	Instance->SetPosition(SavedPosition, false);
	Instance->SetPlayRate(SavedPlayRate);
	Instance->SetPlaying(bSavedPlaying);
	Instance->SetLooping(bSavedLooping);
}

void FSingleAnimationPlayData::PopulateFrom(UAnimSingleNodeInstance* Instance)
{
	AnimToPlay = Instance->CurrentAsset;
	SavedPosition = Instance->CurrentTime;
	SavedPlayRate = Instance->PlayRate;
	bSavedPlaying = Instance->bPlaying;
	bSavedLooping = Instance->bLooping;
}

void FSingleAnimationPlayData::ValidatePosition()
{
	float Min=0,Max=0;

	if (AnimToPlay)
	{
		UAnimSequenceBase* SequenceBase = Cast<UAnimSequenceBase>(AnimToPlay);
		if (SequenceBase)
		{
			Max = SequenceBase->SequenceLength;
		}
	}
	else if (VertexAnimToPlay)
	{
		Max = VertexAnimToPlay->GetAnimLength();
	}

	SavedPosition = FMath::Clamp<float>(SavedPosition, Min, Max);
}

FTransform USkeletalMeshComponent::ConvertLocalRootMotionToWorld(const FTransform & InTransform)
{
	// Make sure component to world is up to date
	if (!bWorldToComponentUpdated)
	{
		UpdateComponentToWorld();
	}

	const FTransform NewWorldTransform = InTransform * ComponentToWorld;
	const FVector DeltaWorldTranslation = NewWorldTransform.GetTranslation() - ComponentToWorld.GetTranslation();
	const FQuat DeltaWorldRotation = ComponentToWorld.GetRotation().Inverse() * NewWorldTransform.GetRotation();

	const FTransform DeltaWorldTransform(DeltaWorldRotation, DeltaWorldTranslation);

	UE_LOG(LogRootMotion, Log,  TEXT("ConvertLocalRootMotionToWorld LocalT: %s, LocalR: %s, WorldT: %s, WorldR: %s."),
		*InTransform.GetTranslation().ToCompactString(), *InTransform.GetRotation().Rotator().ToCompactString(), 
		*DeltaWorldTransform.GetTranslation().ToCompactString(), *DeltaWorldTransform.GetRotation().Rotator().ToCompactString() );

	return DeltaWorldTransform;
}


float USkeletalMeshComponent::CalculateMass() const 
{
	if (bUseSingleBodyPhysics)
	{
		return UPrimitiveComponent::CalculateMass();
	}
	else
	{
		float Mass = 0.0f;

		for (int32 i=0; i < Bodies.Num(); ++i)
		{
			if (Bodies[i]->BodySetup.IsValid())
			{
				Mass += Bodies[i]->BodySetup->CalculateMass(this);
			}
		}

		return Mass;
	}
}

#if WITH_EDITOR

void USkeletalMeshComponent::UpdateCollisionProfile()
{
	Super::UpdateCollisionProfile();

	for(int32 i=0; i < Bodies.Num(); ++i)
	{
		if(Bodies[i]->BodySetup.IsValid())
		{
			Bodies[i]->LoadProfileData(false);
		}
	}
}

void USkeletalMeshComponent::RegisterOnSkeletalMeshPropertyChanged( const FOnSkeletalMeshPropertyChanged& Delegate )
{
	OnSkeletalMeshPropertyChanged.Add(Delegate);
}

void USkeletalMeshComponent::UnregisterOnSkeletalMeshPropertyChanged( const FOnSkeletalMeshPropertyChanged& Delegate )
{
	OnSkeletalMeshPropertyChanged.Remove(Delegate);
}

void USkeletalMeshComponent::ValidateAnimation()
{
	if(AnimationMode == EAnimationMode::AnimationSingleNode)
	{
		if(AnimationData.AnimToPlay && SkeletalMesh && AnimationData.AnimToPlay->GetSkeleton() != SkeletalMesh->Skeleton)
		{
			UE_LOG(LogAnimation, Warning, TEXT("Animation %s is incompatible with skeleton %s, removing animation from actor."), *AnimationData.AnimToPlay->GetName(), *SkeletalMesh->Skeleton->GetName());
			AnimationData.AnimToPlay = NULL;
		}
	}
	else
	{
		if(AnimBlueprintGeneratedClass && SkeletalMesh && AnimBlueprintGeneratedClass->TargetSkeleton != SkeletalMesh->Skeleton)
		{
			UE_LOG(LogAnimation, Warning, TEXT("AnimBP %s is incompatible with skeleton %s, removing AnimBP from actor."), *AnimBlueprintGeneratedClass->GetName(), *SkeletalMesh->Skeleton->GetName());
			AnimBlueprintGeneratedClass = NULL;
		}
	}
}

#endif
