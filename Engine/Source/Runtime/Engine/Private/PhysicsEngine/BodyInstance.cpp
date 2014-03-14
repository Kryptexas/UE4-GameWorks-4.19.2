// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	BodyInstance.cpp
=============================================================================*/ 

#include "EnginePrivate.h"
#include "Collision.h"

#include "MessageLog.h"

#if WITH_PHYSX
	#include "PhysXSupport.h"
	#include "../Collision/PhysXCollision.h"
	#include "../Collision/CollisionConversions.h"
#endif // WITH_PHYSX

#define LOCTEXT_NAMESPACE "BodyInstance"


////////////////////////////////////////////////////////////////////////////
// FCollisionResponse
////////////////////////////////////////////////////////////////////////////

FCollisionResponse::FCollisionResponse()
{

}

FCollisionResponse::FCollisionResponse(ECollisionResponse DefaultResponse)
{
	SetAllChannels(DefaultResponse);
}

/** Set the response of a particular channel in the structure. */
void FCollisionResponse::SetResponse(ECollisionChannel Channel, ECollisionResponse NewResponse)
{
#if 1// @hack until PostLoad is disabled for CDO of BP WITH_EDITOR
	ECollisionResponse DefaultResponse = FCollisionResponseContainer::GetDefaultResponseContainer().GetResponse(Channel);
	if (DefaultResponse == NewResponse)
	{
		RemoveReponseFromArray(Channel);
	}
	else
	{
		AddReponseToArray(Channel, NewResponse);
	}
#endif

	ResponseToChannels.SetResponse(Channel, NewResponse);
}

/** Set all channels to the specified response */
void FCollisionResponse::SetAllChannels(ECollisionResponse NewResponse)
{
	ResponseToChannels.SetAllChannels(NewResponse);
#if 1// @hack until PostLoad is disabled for CDO of BP WITH_EDITOR
	UpdateArrayFromResponseContainer();
#endif
}

/** Returns the response set on the specified channel */
ECollisionResponse FCollisionResponse::GetResponse(ECollisionChannel Channel) const
{
	return ResponseToChannels.GetResponse(Channel);
}

/** Set all channels from ChannelResponse Array **/
void FCollisionResponse::SetCollisionResponseContainer(const FCollisionResponseContainer & InResponseToChannels)
{
	ResponseToChannels = InResponseToChannels;
#if 1// @hack until PostLoad is disabled for CDO of BP WITH_EDITOR
	// this is only valid case that has to be updated
	UpdateArrayFromResponseContainer();
#endif
}

void FCollisionResponse::SetResponsesArray(const TArray<FResponseChannel> & InChannelResponses)
{
#if DO_GUARD_SLOW
	// verify if the name is overlapping, if so, ensure, do not remove in debug becuase it will cause inconsistent bug between debug/release
	int32 const ResponseNum = InChannelResponses.Num();
	for (int32 I=0; I<ResponseNum; ++I)
	{
		for (int32 J=I+1; J<ResponseNum; ++J)
		{
			if (InChannelResponses[I].Channel == InChannelResponses[J].Channel)
			{
				UE_LOG(LogCollision, Warning, TEXT("Collision Channel : Redundant name exists"));
			}
		}
	}
#endif

	ResponseArray = InChannelResponses;
	UpdateResponseContainerFromArray();
}

#if 1// @hack until PostLoad is disabled for CDO of BP WITH_EDITOR
bool FCollisionResponse::RemoveReponseFromArray(ECollisionChannel Channel)
{
	// this is expensive operation, I'd love to remove names but this operation is supposed to do
	// so only allow it in editor
	// without editor, this does not have to match 
	// We'd need to save name just in case that name is gone or not
	FName ChannelName = UCollisionProfile::Get()->ReturnChannelNameFromContainerIndex(Channel);
	for (auto Iter=ResponseArray.CreateIterator(); Iter; ++Iter)
	{
		if (ChannelName == (*Iter).Channel)
		{
			ResponseArray.RemoveAt(Iter.GetIndex());
			return true;
		}
	}
	return false;
}

bool FCollisionResponse::AddReponseToArray(ECollisionChannel Channel, ECollisionResponse Response)
{
	// this is expensive operation, I'd love to remove names but this operation is supposed to do
	// so only allow it in editor
	// without editor, this does not have to match 
	FName ChannelName = UCollisionProfile::Get()->ReturnChannelNameFromContainerIndex(Channel);
	for (auto Iter=ResponseArray.CreateIterator(); Iter; ++Iter)
	{
		if (ChannelName == (*Iter).Channel)
		{
			(*Iter).Response = Response;
			return true;
		}
	}

	// if not add to the list
	ResponseArray.Add(FResponseChannel(ChannelName, Response));
	return true;
}

void FCollisionResponse::UpdateArrayFromResponseContainer()
{
	ResponseArray.Empty();

	const FCollisionResponseContainer & DefaultResponse = FCollisionResponseContainer::GetDefaultResponseContainer();

	for(int32 i=0; i<ARRAY_COUNT(ResponseToChannels.EnumArray); i++)
	{
		// if not same as default
		if ( ResponseToChannels.EnumArray[i] != DefaultResponse.EnumArray[i] )
		{
			FName ChannelName = UCollisionProfile::Get()->ReturnChannelNameFromContainerIndex(i);
			if (ChannelName != NAME_None)
			{
				FResponseChannel NewResponse;
				NewResponse.Channel = ChannelName;
				NewResponse.Response = (ECollisionResponse)ResponseToChannels.EnumArray[i];
				ResponseArray.Add(NewResponse);
			}
		}
	}
}

#endif // WITH_EDITOR

void FCollisionResponse::UpdateResponseContainerFromArray()
{
	ResponseToChannels = FCollisionResponseContainer::GetDefaultResponseContainer();

	for (auto Iter = ResponseArray.CreateIterator(); Iter; ++Iter)
	{
		FResponseChannel & Response = *Iter;

		int32 EnumIndex = UCollisionProfile::Get()->ReturnContainerIndexFromChannelName(Response.Channel);
		if ( EnumIndex != INDEX_NONE )
		{
			ResponseToChannels.SetResponse((ECollisionChannel)EnumIndex, Response.Response);
		}
		else
		{
			// otherwise remove
			ResponseArray.RemoveAt(Iter.GetIndex());
			--Iter;
		}
	}
}
////////////////////////////////////////////////////////////////////////////
FArchive& operator<<(FArchive& Ar,FBodyInstance& BodyInst)
{
	if (!Ar.IsLoading() && !Ar.IsSaving())
	{
		Ar << BodyInst.OwnerComponent;
		Ar << BodyInst.PhysMaterialOverride;
	}

	return Ar;
}


/** Util for finding the parent bodyinstance of a specified body, using skeleton hierarchy */
FBodyInstance* FindParentBodyInstance(FName BodyName, USkeletalMeshComponent* SkelMeshComp)
{
	FName TestBoneName = BodyName;
	while(true)
	{
		TestBoneName = SkelMeshComp->GetParentBone(TestBoneName);
		// Bail out if parent bone not found
		if(TestBoneName == NAME_None)
		{
			return NULL;
		}

		// See if we have a body for the parent bone
		FBodyInstance* BI = SkelMeshComp->GetBodyInstance(TestBoneName);
		if(BI != NULL)
		{
			// We do - return it
			return BI;
		}

		// Don't repeat if we are already at the root!
		if(SkelMeshComp->GetBoneIndex(TestBoneName) == 0)
		{
			return NULL;
		}
	}

	return NULL;
}

void FBodyInstance::UpdateFromDeprecatedEnableCollision()
{
	//@todo should I invalidate profile name if this happens?
	if(bEnableCollision_DEPRECATED)
	{
		CollisionEnabled = ECollisionEnabled::QueryAndPhysics;
	}
	else
	{
		CollisionEnabled = ECollisionEnabled::NoCollision;
	}
}

#if WITH_PHYSX
TArray<PxShape*> FBodyInstance::GetAllShapes(int32& OutNumSyncShapes) const
{
	// grab shapes from sync actor
	TArray<PxShape*> PSyncShapes;
	if( RigidActorSync != NULL )
	{
		PSyncShapes.AddZeroed( RigidActorSync->getNbShapes() );
		RigidActorSync->getShapes(PSyncShapes.GetData(), PSyncShapes.Num());
	}

	// grab shapes from async actor
	TArray<PxShape*> PAsyncShapes;
	if( RigidActorAsync != NULL )
	{
		SCOPED_SCENE_READ_LOCK( RigidActorAsync->getScene() );
		PAsyncShapes.AddZeroed( RigidActorAsync->getNbShapes() );
		RigidActorAsync->getShapes(PAsyncShapes.GetData(), PAsyncShapes.Num());
	}

	OutNumSyncShapes = PSyncShapes.Num();

	TArray<PxShape*> PAllShapes = PSyncShapes;
	PAllShapes.Append(PAsyncShapes);
	return PAllShapes;
}
#endif

void FBodyInstance::UpdatePhysicalMaterials()
{
#if WITH_PHYSX

	UPhysicalMaterial* SimplePhysMat = GetSimplePhysicalMaterial();
	check(SimplePhysMat != NULL);
	PxMaterial* PSimpleMat = SimplePhysMat->GetPhysXMaterial();
	check(PSimpleMat != NULL);

	if (RigidActorSync != NULL)
	{
		SCENE_LOCK_WRITE(RigidActorSync->getScene());
	}

	if (RigidActorAsync != NULL)
	{
		SCENE_LOCK_WRITE(RigidActorAsync->getScene());
	}

	int32 NumSyncShapes = 0;
	TArray<PxShape*> AllShapes = GetAllShapes(NumSyncShapes);

	for(int32 ShapeIdx=0; ShapeIdx<AllShapes.Num(); ShapeIdx++)
	{
		PxShape* PShape = AllShapes[ShapeIdx];

		// If a triangle mesh, need to get array of materials...
		if(PShape->getGeometryType() == PxGeometryType::eTRIANGLEMESH)
		{
			TArray<UPhysicalMaterial*> ComplexPhysMats = GetComplexPhysicalMaterials();
			TArray<PxMaterial*> PComplexMats;
			PComplexMats.AddUninitialized( ComplexPhysMats.Num() );
			for(int MatIdx=0; MatIdx<ComplexPhysMats.Num(); MatIdx++)
			{
				check(ComplexPhysMats[MatIdx] != NULL);
				PComplexMats[MatIdx] = ComplexPhysMats[MatIdx]->GetPhysXMaterial();
				check(PComplexMats[MatIdx] != NULL);
			}

			if (PComplexMats.Num())
			{
				PShape->setMaterials(PComplexMats.GetData(), PComplexMats.Num());
			}
			else
			{
				UE_LOG(LogPhysics, Warning, TEXT("FBodyInstance::UpdatePhysicalMaterials : PComplexMats is empty - falling back on simple physical material."));
				PShape->setMaterials(&PSimpleMat, 1);
			}

		}
		// Simple shape, 
		else
		{
			PShape->setMaterials(&PSimpleMat, 1);
		}
	}

	if (RigidActorSync != NULL)
	{
		SCENE_UNLOCK_WRITE(RigidActorSync->getScene());
	}

	if (RigidActorAsync != NULL)
	{
		SCENE_UNLOCK_WRITE(RigidActorAsync->getScene());
	}
#endif
}

void FBodyInstance::InvalidateCollisionProfileName()
{
	if ( CollisionProfileName != NAME_None )
	{
		CollisionProfileName = NAME_None;
	}
}

ECollisionResponse FBodyInstance::GetResponseToChannel(ECollisionChannel Channel) const
{
	return CollisionResponses.GetResponse(Channel);
}

void FBodyInstance::SetResponseToChannel(ECollisionChannel Channel, ECollisionResponse NewResponse)
{
	InvalidateCollisionProfileName();
	ResponseToChannels_DEPRECATED.SetResponse(Channel, NewResponse);
	CollisionResponses.SetResponse(Channel, NewResponse);
	UpdatePhysicsFilterData();
}

const FCollisionResponseContainer & FBodyInstance::GetResponseToChannels() const
{
	return CollisionResponses.GetResponseContainer();
}

void FBodyInstance::SetResponseToAllChannels(ECollisionResponse NewResponse)
{
	InvalidateCollisionProfileName();
	ResponseToChannels_DEPRECATED.SetAllChannels(NewResponse);
	CollisionResponses.SetAllChannels(NewResponse);
	UpdatePhysicsFilterData();
}
	
void FBodyInstance::SetResponseToChannels(const FCollisionResponseContainer& NewReponses)
{
	InvalidateCollisionProfileName();
	CollisionResponses.SetCollisionResponseContainer(NewReponses);
	UpdatePhysicsFilterData();
}
	
void FBodyInstance::SetObjectType(ECollisionChannel Channel)
{
	InvalidateCollisionProfileName();
	ObjectType = Channel;
	UpdatePhysicsFilterData();
}

ECollisionChannel FBodyInstance::GetObjectType() const
{
	return ObjectType;
}

void FBodyInstance::SetCollisionProfileName(FName InCollisionProfileName)
{
	if ( CollisionProfileName!=InCollisionProfileName )
	{
		CollisionProfileName = InCollisionProfileName;
		// now Load ProfileData
		LoadProfileData(false);
	}
}

FName FBodyInstance::GetCollisionProfileName() const
{
	return CollisionProfileName;
}


void FBodyInstance::SetCollisionEnabled(ECollisionEnabled::Type NewType, bool bUpdatePhysicsFilterData)
{
	if (CollisionEnabled != NewType)
	{
		InvalidateCollisionProfileName();
		CollisionEnabled = NewType;
		
		// Only update physics filter data if required
		if (bUpdatePhysicsFilterData)
		{
			UpdatePhysicsFilterData();
		}
	}
}

ECollisionEnabled::Type FBodyInstance::GetCollisionEnabled() const
{
	// Check actor override
	AActor* Owner = OwnerComponent.IsValid() ? OwnerComponent.Get()->GetOwner() : NULL;
	if(Owner != NULL && !Owner->GetActorEnableCollision())
	{
		return ECollisionEnabled::NoCollision;
	}
	else
	{
		return CollisionEnabled;
	}
}


/** Update the filter data on the physics shapes, based on the owning component flags. */
void FBodyInstance::UpdatePhysicsFilterData()
{
#if WITH_PHYSX
	// Do nothing if no physics actor
	if(GetPxRigidActor() == NULL)
	{
		return;
	}

	check(BodySetup.IsValid());

	// Figure out if we are static
	AActor* Owner = OwnerComponent.IsValid() ? OwnerComponent.Get()->GetOwner() : NULL;
	int32 OwnerID = (Owner != NULL) ? Owner->GetUniqueID() : 0;
	const bool bPhysicsStatic = !OwnerComponent.IsValid() || OwnerComponent.Get()->IsWorldGeometry();

	// Grab collision setting from body instance
	TEnumAsByte<ECollisionEnabled::Type> UseCollisionEnabled = GetCollisionEnabled(); // this checks actor override
	bool bUseNotifyRBCollision = bNotifyRigidBodyCollision;
	FCollisionResponseContainer UseResponse = CollisionResponses.GetResponseContainer();

	// Get skelmeshcomp ID
	uint32 SkelMeshCompID = 0;
	USkeletalMeshComponent* SkelMeshComp = Cast<USkeletalMeshComponent>(OwnerComponent.Get());
	if(SkelMeshComp != NULL && !SkelMeshComp->bUseSingleBodyPhysics)
	{
		SkelMeshCompID = SkelMeshComp->GetUniqueID();

		// In skeletal case, collision enable/disable/movement should be overriden by mesh component
		// being in the physics asset, and not having collision is a waste and it can cause a bug where disconnected bodies
		UseCollisionEnabled = SkelMeshComp->BodyInstance.CollisionEnabled; 
		ObjectType = SkelMeshComp->GetCollisionObjectType();

		if (BodySetup->CollisionReponse == EBodyCollisionResponse::BodyCollision_Enabled)
		{
			UseResponse.SetAllChannels(ECR_Block);
		}
		else if (BodySetup->CollisionReponse == EBodyCollisionResponse::BodyCollision_Disabled)
		{
			UseResponse.SetAllChannels(ECR_Ignore);
		}

		UseResponse = FCollisionResponseContainer::CreateMinContainer(UseResponse, SkelMeshComp->BodyInstance.CollisionResponses.GetResponseContainer());
		bUseNotifyRBCollision = bUseNotifyRBCollision && SkelMeshComp->BodyInstance.bNotifyRigidBodyCollision;
	}

#if WITH_EDITOR
	// if no collision, but if world wants to enable trace collision for components, allow it
	if(UseCollisionEnabled == ECollisionEnabled::NoCollision && Owner && Owner->IsA(AVolume::StaticClass())==false )
	{
		UWorld * World = Owner->GetWorld();
		UPrimitiveComponent* PrimComp = OwnerComponent.Get();
		if (World && World->bEnableTraceCollision && 
			(PrimComp->IsA(UStaticMeshComponent::StaticClass()) || PrimComp->IsA(USkeletalMeshComponent::StaticClass()) || PrimComp->IsA(UBrushComponent::StaticClass())))
		{
			//UE_LOG(LogPhysics, Warning, TEXT("Enabling collision %s : %s"), *GetNameSafe(Owner), *GetNameSafe(OwnerComponent.Get()));
			// clear all other channel just in case other people using those channels to do something
			UseResponse.SetAllChannels(ECR_Ignore);
			UseResponse.SetResponse(ECC_Visibility, ECR_Block);
			UseCollisionEnabled = ECollisionEnabled::QueryOnly;
		}
	}
#endif

	// Is the target a static actor
	bool bDestStatic = (GetPxRigidActor()->isRigidStatic() != NULL);
	bool bUseComplexAsSimple = (BodySetup.Get()->CollisionTraceFlag==CTF_UseComplexAsSimple);
	bool bUseSimpleAsComplex = (BodySetup.Get()->CollisionTraceFlag==CTF_UseSimpleAsComplex);

	// Create the filterdata structs
	PxFilterData PQueryFilterData, PSimFilterData;
	PxFilterData PSimpleQueryData, PComplexQueryData; 
	if (UseCollisionEnabled != ECollisionEnabled::NoCollision)
	{
		CreateShapeFilterData(ObjectType, OwnerID, UseResponse, SkelMeshCompID, InstanceBodyIndex, PQueryFilterData, PSimFilterData, bUseCCD && !bPhysicsStatic, bUseNotifyRBCollision, bPhysicsStatic);
		PSimpleQueryData = PQueryFilterData;
		PComplexQueryData = PQueryFilterData;	
		
		// Build filterdata variations for complex and simple
		PSimpleQueryData.word3 |= EPDF_SimpleCollision;
		if(bUseSimpleAsComplex)
		{
			PSimpleQueryData.word3 |= EPDF_ComplexCollision;
		}

		PComplexQueryData.word3 |= EPDF_ComplexCollision;
		if(bUseComplexAsSimple)
		{
			PComplexQueryData.word3 |= EPDF_SimpleCollision;
		}		
	}

	// Iterate over all shapes and assign filterdata
	int32 NumSyncShapes = 0;
	TArray<PxShape*> AllShapes = GetAllShapes(NumSyncShapes);

	bool bUpdateMassProperties = false;

	// Only perform scene queries in the synchronous scene for static shapes
	const int32 SceneQueryShapeNumMax = bDestStatic ? NumSyncShapes : AllShapes.Num();

	PxScene* AsyncScene = (RigidActorAsync != NULL) ? RigidActorAsync->getScene() : NULL;
	SCENE_LOCK_WRITE(AsyncScene);

	for(int32 ShapeIdx=0; ShapeIdx<AllShapes.Num(); ShapeIdx++)
	{
		PxShape* PShape = AllShapes[ShapeIdx];
		PShape->setSimulationFilterData(PSimFilterData);

		// If query collision is enabled..
		if(UseCollisionEnabled == ECollisionEnabled::QueryAndPhysics || UseCollisionEnabled == ECollisionEnabled::QueryOnly)
		{
			// Only perform scene queries in the synchronous scene for static shapes
			if(bPhysicsStatic)
			{
				bool bIsSyncShape = (ShapeIdx < NumSyncShapes);
				PShape->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, bIsSyncShape);
			}
			// If non-static, always enable scene queries
			else
			{
				PShape->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, true);
			}

			// See if we want physics collision
			bool bSimCollision = (UseCollisionEnabled == ECollisionEnabled::QueryAndPhysics);

			// Triangle mesh is 'complex' geom
			if(PShape->getGeometryType() == PxGeometryType::eTRIANGLEMESH)
			{
				PShape->setQueryFilterData(PComplexQueryData);

				// on dynamic objects and objects which don't use complex as simple, tri mesh not used for sim
				if(!bSimCollision || !bDestStatic || !bUseComplexAsSimple)
				{
					PShape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false); 
				}
				else
				{
					PShape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true); 
				}

				if(OwnerComponent == NULL || !OwnerComponent->IsA(UModelComponent::StaticClass()))
				{
					PShape->setFlag(PxShapeFlag::eVISUALIZATION, false); // dont draw the tri mesh, we can see it anyway, and its slow
				}
			}
			// Everything else is 'simple'
			else
			{
				PShape->setQueryFilterData(PSimpleQueryData);

				// See if we currently have sim collision
				bool bCurrentSimCollision = (PShape->getFlags() & PxShapeFlag::eSIMULATION_SHAPE);
				// Enable sim collision
				if (bSimCollision && !bCurrentSimCollision)
				{
					bUpdateMassProperties = true;
					PShape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true); 
				}
				// Disable sim collision
				else if(!bSimCollision && bCurrentSimCollision)
				{
					bUpdateMassProperties = true;
					PShape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);
				}
							
				// enable swept bounds for CCD for this shape
				PxRigidBody* PBody = GetPxRigidActor()->is<PxRigidBody>();
				if(bSimCollision && !bPhysicsStatic && bUseCCD && PBody)
				{
					PBody->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, true);
				}
				else if(PBody)
				{
					
					PBody->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, false);
				}
			}
		}
		// No collision enabled
		else
		{
			PShape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false); 
			PShape->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, false); 	
		}
	}

	if (bUpdateMassProperties)
	{
		UpdateMassProperties();
	}

	SCENE_UNLOCK_WRITE(AsyncScene);
#endif // WITH_PHYSX
}

#if WITH_PHYSX
void FBodyInstance::InitBody(UBodySetup* Setup, const FTransform& Transform, UPrimitiveComponent* PrimComp, FPhysScene* InRBScene, PxAggregate* InAggregate)
{
	PhysxUserData = FPhysxUserData(this);

	check(Setup);
	AActor* Owner = PrimComp ? PrimComp->GetOwner() : NULL;

	// If there is already a body instanced, or there is no scene to create it into, do nothing.
	if (GetPxRigidActor() != NULL || !InRBScene)
	{
		return;
	}

	// Make the debug name for this geometry...
	FString DebugName(TEXT(""));
#if (WITH_EDITORONLY_DATA || UE_BUILD_DEBUG || LOOKING_FOR_PERF_ISSUES) && !(UE_BUILD_SHIPPING || UE_BUILD_TEST) && !NO_LOGGING
	UStaticMeshComponent* SMComp = Cast<UStaticMeshComponent>(PrimComp);
	if(Owner)
	{
		DebugName += FString::Printf( TEXT("Actor: %s "), *Owner->GetPathName() );
	}

	if(PrimComp)
	{
		DebugName += FString::Printf( TEXT("Component: %s "), *PrimComp->GetName() );
	}

	if(SMComp)
	{
		DebugName += FString::Printf( TEXT("StaticMesh: %s"), *SMComp->StaticMesh->GetPathName() );
	}

	if(Setup->BoneName != NAME_None)
	{
		DebugName += FString::Printf( TEXT("Bone: %s "), *Setup->BoneName.ToString() );
	}
	// Convert to char* for PhysX
	CharDebugName = MakeShareable(new TArray<ANSICHAR>(StringToArray<ANSICHAR>(*DebugName, DebugName.Len() + 1)));
#endif

	if(Transform.GetScale3D().IsNearlyZero())
	{
		UE_LOG(LogPhysics, Warning, TEXT("FBodyInstance::InitBody : Scale3D is (nearly) zero: %s"), *DebugName);
		return;
	}

	// Check we support mirroring/non-mirroring
	const float TransformDet = Transform.GetDeterminant();
	if(TransformDet < 0.f && !Setup->bGenerateMirroredCollision)
	{
		UE_LOG(LogPhysics, Warning, TEXT("FBodyInstance::InitBody : Body is mirrored but bGenerateMirroredCollision == false: %s"), *DebugName);
		return;
	}

	if(TransformDet > 0.f && !Setup->bGenerateNonMirroredCollision)
	{
		UE_LOG(LogPhysics, Warning, TEXT("FBodyInstance::InitBody : Body is not mirrored but bGenerateNonMirroredCollision == false: %s"), *DebugName);
		return;
	}

	if(Transform.ContainsNaN())
	{
		UE_LOG(LogPhysics, Warning, TEXT("InitBody: Bad transform - %s %s\n%s"), *DebugName, *Setup->BoneName.ToString(), *Transform.ToString());
		return;
	}

	// remember my owning component
	OwnerComponent	= PrimComp;
	BodySetup		= Setup;

	// whenever update filter, this check will trigger, it seems it's best to know when we initialize body instead. 
	check(BodySetup.IsValid());

	USkeletalMeshComponent* SkelMeshComp = NULL;
	if (OwnerComponent.IsValid() && OwnerComponent.Get()->IsA(USkeletalMeshComponent::StaticClass()))
	{
		SkelMeshComp = Cast<USkeletalMeshComponent>(OwnerComponent.Get());

		// if not dedicated and if set up is done to use physics, enable flag on
		if (BodySetup->PhysicsType == PhysType_Unfixed)
		{
			bool bEnableSimulation = (SkelMeshComp && IsRunningDedicatedServer())? SkelMeshComp->bEnablePhysicsOnDedicatedServer: true;
			if ( bEnableSimulation )
			{
				// set simulate to true if using physics
				bSimulatePhysics = true;
				PhysicsBlendWeight = 1.f;
			}
	 		else
	 		{
	 			bSimulatePhysics = false;
	 			PhysicsBlendWeight = 0.f;
	 		}
		}
	}

	// See if we are 'static'
	const bool bPhysicsStatic = (OwnerComponent == NULL) || (OwnerComponent->Mobility != EComponentMobility::Movable);

	// In skeletal case, we need both our bone and skelcomponent flag to be true.
	// This might be 'and'ing us with ourself, but thats fine.
	const bool bUseSimulate = ShouldInstanceSimulatingPhysics();

	PxScene* PSceneForNewDynamic = NULL;	// TBD
	PxScene* PSceneSync = InRBScene->GetPhysXScene(PST_Sync);
	PxScene* PSceneAsync = InRBScene->HasAsyncScene() ? InRBScene->GetPhysXScene(PST_Async) : NULL;

	PxRigidDynamic* PNewDynamic = NULL;
	PxRigidActor* PNewActorSync = NULL;
	PxRigidActor* PNewActorAsync = NULL;

	PxTransform PTransform = U2PTransform(Transform);

	if(bPhysicsStatic)
	{
		// Put the static actor in both scenes
		PNewActorSync = GPhysXSDK->createRigidStatic(PTransform);
		if (PSceneAsync)
		{
			PNewActorAsync = GPhysXSDK->createRigidStatic(PTransform);
		}
	}
	else
	{
		PNewDynamic = GPhysXSDK->createRigidDynamic(PTransform);

		// Put the dynamic actor in one scene or the other
		if( !UseAsyncScene() )
		{
			PNewActorSync = PNewDynamic;
			PSceneForNewDynamic = PSceneSync;
		}
		else
		{
			PNewActorAsync = PNewDynamic;
			PSceneForNewDynamic = PSceneAsync;
		}

		// Set kinematic if desired
		if(bUseSimulate)
		{
			PNewDynamic->setRigidDynamicFlag(PxRigidDynamicFlag::eKINEMATIC, false);
		}
		else
		{
			PNewDynamic->setRigidDynamicFlag(PxRigidDynamicFlag::eKINEMATIC, true);
		}

		// turn off gravity if desired
		if (!bEnableGravity)
		{
			PNewDynamic->setActorFlag( PxActorFlag::eDISABLE_GRAVITY, true );
		}

		// If we ever drive this body kinematically, we want to use its target for scene queries, so collision is updated right away, not on the next physics sim
		PNewDynamic->setRigidDynamicFlag(PxRigidDynamicFlag::eUSE_KINEMATIC_TARGET_FOR_SCENE_QUERIES, true);
	}


	// save Scale3D 
	Scale3D = Transform.GetScale3D();


	// Copy geom from template and scale

	// Sync:
	if( PNewActorSync != NULL )
	{
		Setup->AddShapesToRigidActor(PNewActorSync, Scale3D);
		PNewActorSync->userData = &PhysxUserData; // Store pointer to owning bodyinstance.
		PNewActorSync->setName( CharDebugName.IsValid() ? CharDebugName->GetTypedData() : NULL );

		check(FPhysxUserData::Get<FBodyInstance>(PNewActorSync->userData) == this && FPhysxUserData::Get<FBodyInstance>(PNewActorSync->userData)->OwnerComponent != NULL);
	}

	// Async:
	if( PNewActorAsync != NULL )
	{
		check(PSceneAsync);

		Setup->AddShapesToRigidActor(PNewActorAsync, Scale3D);
		PNewActorAsync->userData = &PhysxUserData; // Store pointer to owning bodyinstance.
		PNewActorAsync->setName( CharDebugName.IsValid() ? CharDebugName->GetTypedData() : NULL );

		check(FPhysxUserData::Get<FBodyInstance>(PNewActorAsync->userData) == this && FPhysxUserData::Get<FBodyInstance>(PNewActorAsync->userData)->OwnerComponent != NULL);
	}

	// If we added no shapes, generate warning, destroy actor and bail out (don't add to scene).
	if((PNewActorSync && PNewActorSync->getNbShapes() == 0) || ((PNewActorAsync && PNewActorAsync->getNbShapes() == 0)))
	{
		if (DebugName.Len())
		{
			UE_LOG(LogPhysics, Log, TEXT("InitBody: failed - no shapes: %s"), *DebugName);
		}
		// else if probably a level with no bsp
		if(PNewActorSync)
		{
			PNewActorSync->release();
		}
		if(PNewActorAsync)
		{
			PNewActorAsync->release();
		}

		//clear Owner and Setup info as well to properly clean up the BodyInstance.
		OwnerComponent	= NULL;
		BodySetup		= NULL;

		return;
	}

	// Store pointers to PhysX data in RB_BodyInstance
	RigidActorSync = PNewActorSync;
	RigidActorAsync = PNewActorAsync;

	// Store scene indices
	SceneIndexSync = InRBScene->PhysXSceneIndex[PST_Sync];
	SceneIndexAsync = PSceneAsync ? InRBScene->PhysXSceneIndex[PST_Async] : 0;

	// Apply correct physical materials to shape we created.
	UpdatePhysicalMaterials();

	// Set the filter data on the shapes (call this after setting BodyData because it uses that pointer)
	UpdatePhysicsFilterData();

	// Need to add actor into scene before calling putToSleep
	// check if InAggregate is passed in
	// BRG N.B. : For now only using InAggregate and BodyAggregate for dynamic bodies that are in the same scene as the aggregate, since otherwise we might have actors from two scenes.
	// Right now aggregates are effectively disabled anyhow.
	if(InAggregate && PNewDynamic != NULL && InAggregate->getScene() == PSceneForNewDynamic)
	{
		InAggregate->addActor(*PNewDynamic);
	}
	else if(PNewDynamic != NULL && PNewDynamic->getNbShapes()  > AggregateBodyShapesThreshold)		// a lot of shapes for a single body case
	{
		BodyAggregate = GPhysXSDK->createAggregate(AggregateMaxSize, true);
		if(BodyAggregate)
		{
			BodyAggregate->addActor(*PNewDynamic);
			SCOPED_SCENE_WRITE_LOCK(PSceneForNewDynamic);
			PSceneForNewDynamic->addAggregate(*BodyAggregate);
		}
	}
	else
	{
		// Actually add actor(s) to scene(s) (if not artic link)
		if(PNewActorSync != NULL)
		{
			SCOPED_SCENE_WRITE_LOCK(PSceneSync);
			PSceneSync->addActor(*PNewActorSync);
		}
		if(PNewActorAsync != NULL)
		{
			SCOPED_SCENE_WRITE_LOCK(PSceneAsync);
			PSceneAsync->addActor(*PNewActorAsync);
		}
	}


	// Set initial velocities
	FVector InitialLinVel(0.f);
	if(Owner != NULL)
	{	
		InitialLinVel = Owner->GetVelocity();
	}

	if(PNewDynamic != NULL)
	{
		// Compute mass (call this after setting BodyData because it uses that pointer)
		UpdateMassProperties();
		// Update damping
		UpdateDampingProperties();

		// Set initial velocity 
		if(bUseSimulate)
		{
			PNewDynamic->setLinearVelocity( U2PVector(InitialLinVel) );
		}

		// Set the parameters for determining when to put the object to sleep.
		float SleepEnergyThresh = PNewDynamic->getSleepThreshold();
		if( SleepFamily == SF_Sensitive)
		{
			SleepEnergyThresh /= 20.f;
		}
		PNewDynamic->setSleepThreshold(SleepEnergyThresh);
		// set solver iteration count 
		int32 PositionIterCount = FMath::Clamp(PositionSolverIterationCount, 1, 255);
		int32 VelocityIterCount = FMath::Clamp(VelocitySolverIterationCount, 1, 255);
		PNewDynamic->setSolverIterationCounts(PositionIterCount, VelocityIterCount);

		bool bShouldStartAwake = bStartAwake;

		if (SkelMeshComp != NULL)
		{
			bShouldStartAwake = bShouldStartAwake && SkelMeshComp->BodyInstance.bStartAwake;
		}

		// wakeUp and putToSleep will issue warnings on kinematic actors
		if(IsRigidDynamicNonKinematic(PNewDynamic))
		{
		    // Sleep/wake up as appropriate
		    if(bShouldStartAwake || (Owner && Owner->GetVelocity().Size() > KINDA_SMALL_NUMBER))
		    {
			    // Wake up bodies that are part of a moving actor.
			    PNewDynamic->wakeUp();
		    }
		    else
		    {
			    // Bodies should start out sleeping.
			    PNewDynamic->putToSleep();
		    }
		}
	}
}
#endif // WITH_PHYSX

/**
 *	Clean up the physics engine info for this instance.
 */
void FBodyInstance::TermBody()
{
#if WITH_PHYSX
	// Release sync actor if the scene still exists
	PxScene* PSceneSync = GetPhysXSceneFromIndex( SceneIndexSync );

	if( PSceneSync != NULL )
	{
		// Enable scene lock, in case it is required
		SCOPED_SCENE_WRITE_LOCK(PSceneSync);

		if( RigidActorSync )
		{
			// Let FPhysScene know
			FPhysScene * PhysScene = FPhysxUserData::Get<FPhysScene>(PSceneSync->userData);
			if (PhysScene)
			{
				PhysScene->TermBody(this);
			}

			RigidActorSync->release();
			RigidActorSync = NULL;	//we must do this within the lock because we use it in the substepping thread to determine that RigidActorSync is still valid
		}
	}
	
	SceneIndexSync = 0;

	// Release async actor if the scene still exists
	if (SceneIndexAsync != 0)
	{
		PxScene* PSceneAsync = GetPhysXSceneFromIndex( SceneIndexAsync );
		if( PSceneAsync != NULL )
		{
			// Enable scene lock, in case it is required
			SCOPED_SCENE_WRITE_LOCK(PSceneAsync);

			if( RigidActorAsync )
			{
				// Let FPhysScene know
				FPhysScene * PhysScene = FPhysxUserData::Get<FPhysScene>(PSceneAsync->userData);
				if (PhysScene)
				{
					PhysScene->TermBody(this);
				}

				RigidActorAsync->release();
				RigidActorAsync = NULL;
			}
		}
		
		SceneIndexAsync = 0;
	}
	

	BodySetup	= NULL;
	// releasing BodyAggregate, it shouldn't contain RigidActor now, because it's released above
	if(BodyAggregate)
	{
		check(!BodyAggregate->getNbActors());
		BodyAggregate->release();
		BodyAggregate = NULL;
	}
#endif

	// @TODO UE4: Release spring body here

	OwnerComponent = NULL;
}

bool FBodyInstance::UpdateBodyScale(const FVector & inScale3D)
{
#if WITH_PHYSX
	if (BodySetup == NULL || GetPxRigidActor() == NULL)
	{
		//UE_LOG(LogPhysics, Log, TEXT("Body hasn't been initialized. Call InitBody to initialize."));
		return false;
	}

	// if same, return
	if (Scale3D.Equals(inScale3D))
	{
		return false;
	}

	bool bSuccess=false;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	ensure ( !Scale3D.ContainsNaN() && !inScale3D.ContainsNaN() );
#endif
	FVector OldScale3D = Scale3D;

	if (OldScale3D.IsNearlyZero())
	{
		// min scale
		OldScale3D = FVector(0.1f);
	}

	// Determine if applied scaling is uniform. If it isn't, only convex geometry will be copied over
	float	NewUniScale = inScale3D.GetMin()/OldScale3D.GetMin(); // total scale includes inversing old scale and applying new scale
	float	NewUniScaleAbs = FMath::Abs(NewUniScale);

	// Is the target a static actor
	// Not used
//	bool bDestStatic = (RigidActor->isRigidStatic() != NULL);

	// Add up shapes so we don't have to allocate twice
	uint32 TotalShapeCount = 0;

	if(RigidActorSync != NULL)
	{
		SCOPED_SCENE_READ_LOCK(RigidActorSync->getScene());
		TotalShapeCount += RigidActorSync->getNbShapes();
	}

	if(RigidActorAsync != NULL)
	{
		SCOPED_SCENE_READ_LOCK(RigidActorAsync->getScene());
		TotalShapeCount += RigidActorAsync->getNbShapes();
	}

	TArray<PxShape*> PShapes;
	PShapes.AddZeroed(TotalShapeCount);

	// Record the sync actor shapes
	int32 NumShapesSync = 0;
	if( RigidActorSync != NULL )
	{
		SCOPED_SCENE_READ_LOCK(RigidActorSync->getScene());
		NumShapesSync = RigidActorSync->getShapes(PShapes.GetData(), RigidActorSync->getNbShapes());
	}

	// Record the async actor shapes
	int32 NumShapesAsync = 0;
	if( RigidActorAsync != NULL )
	{
		SCOPED_SCENE_READ_LOCK(RigidActorAsync->getScene());
		// Note array start is offset by NumShapesSync
		NumShapesAsync = RigidActorAsync->getShapes(PShapes.GetData() + NumShapesSync, RigidActorAsync->getNbShapes());
	}

	for(int32 ShapeIdx=0; ShapeIdx<PShapes.Num(); ShapeIdx++)
	{
		PxShape* PShape = PShapes[ShapeIdx];
		check(PShape);

		PxScene* PScene = PShape->getActor()->getScene();
		SCENE_LOCK_READ(PScene);

		PxTransform PLocalPose = PShape->getLocalPose();
		PLocalPose.q.normalize();
		PxGeometryType::Enum GeomType = PShape->getGeometryType();
		
		if (GeomType == PxGeometryType::eSPHERE)
		{
			PxSphereGeometry PSphereGeom;
			PShape->getSphereGeometry(PSphereGeom);
			SCENE_UNLOCK_READ(PScene);

			PSphereGeom.radius	*= NewUniScaleAbs;
			PLocalPose.p		*= NewUniScale;

			if (PSphereGeom.isValid())
			{
				SCOPED_SCENE_WRITE_LOCK(PScene);
				PShape->setLocalPose(PLocalPose);
				PShape->setGeometry(PSphereGeom);
				bSuccess = true;
			}
			else
			{
				FMessageLog("PIE").Warning(FText::Format(LOCTEXT("PhysicsInvalidScale", "Scale ''{0}'' is not valid on object '{1}'."), FText::FromString(inScale3D.ToString()), FText::FromString(GetBodyDebugName())));
			}
		}
		else if (GeomType == PxGeometryType::eBOX)
		{
			PxBoxGeometry PBoxGeom;
			PShape->getBoxGeometry(PBoxGeom);
			SCENE_UNLOCK_READ(PScene);

			PBoxGeom.halfExtents.x *= NewUniScaleAbs;
			PBoxGeom.halfExtents.y *= NewUniScaleAbs;
			PBoxGeom.halfExtents.z *= NewUniScaleAbs;
			PLocalPose.p *= NewUniScale;

			if (PBoxGeom.isValid())
			{
				SCOPED_SCENE_WRITE_LOCK(PScene);
				PShape->setGeometry(PBoxGeom);
				PShape->setLocalPose(PLocalPose);
				bSuccess = true;
			}
			else
			{
				FMessageLog("PIE").Warning(FText::Format(LOCTEXT("PhysicsInvalidScale", "Scale ''{0}'' is not valid on object '{1}'."), FText::FromString(inScale3D.ToString()), FText::FromString(GetBodyDebugName())));
			}
		}
		else if (GeomType == PxGeometryType::eCAPSULE)
		{
			PxCapsuleGeometry PCapsuleGeom;
			PShape->getCapsuleGeometry(PCapsuleGeom);
			SCENE_UNLOCK_READ(PScene);

			PCapsuleGeom.halfHeight *= NewUniScaleAbs;
			PCapsuleGeom.radius *= NewUniScaleAbs;
			PLocalPose.p *= NewUniScale;

			if(PCapsuleGeom.isValid())
			{
				SCOPED_SCENE_WRITE_LOCK(PScene);
				PShape->setGeometry(PCapsuleGeom);
				PShape->setLocalPose(PLocalPose);
				bSuccess = true;
			}
			else
			{
				FMessageLog("PIE").Warning(FText::Format(LOCTEXT("PhysicsInvalidScale", "Scale ''{0}'' is not valid on object '{1}'."), FText::FromString(inScale3D.ToString()), FText::FromString(GetBodyDebugName())));
			}
		}
		else if (GeomType == PxGeometryType::eCONVEXMESH)
		{
			PxConvexMeshGeometry PConvexGeom;
			PShape->getConvexMeshGeometry(PConvexGeom);
			SCENE_UNLOCK_READ(PScene);

			// find which convex elems it is
			// it would be nice to know if the order of PShapes array index is in the order of createShape
			// Create convex shapes
			for(int32 i=0; i<BodySetup->AggGeom.ConvexElems.Num(); i++)
			{
				FKConvexElem* ConvexElem = &(BodySetup->AggGeom.ConvexElems[i]);

				// found it
				if(ConvexElem->ConvexMesh == PConvexGeom.convexMesh)
				{
					// Please note that this one we don't inverse old scale, but just set new one
					FVector NewScale3D = inScale3D; 
					FVector Scale3DAbs(FMath::Abs(NewScale3D.X), FMath::Abs(NewScale3D.Y), FMath::Abs(NewScale3D.Z)); // magnitude of scale (sign removed)

					PxTransform PNewLocalPose;
					bool bUseNegX = CalcMeshNegScaleCompensation(NewScale3D, PNewLocalPose);

					PxTransform PElementTransform = U2PTransform(ConvexElem->GetTransform());
					PNewLocalPose.q *= PElementTransform.q;
					PNewLocalPose.p += PElementTransform.p;

					PConvexGeom.convexMesh = bUseNegX ? ConvexElem->ConvexMeshNegX : ConvexElem->ConvexMesh;
					PConvexGeom.scale.scale = U2PVector(Scale3DAbs);

					if(PConvexGeom.isValid())
					{
						SCOPED_SCENE_WRITE_LOCK(PScene);
						PShape->setGeometry(PConvexGeom);
						PShape->setLocalPose(PNewLocalPose);
						bSuccess = true;
					}
					else
					{
						FMessageLog("PIE").Warning(FText::Format(LOCTEXT("PhysicsInvalidScale", "Scale ''{0}'' is not valid on object '{1}'."), FText::FromString(inScale3D.ToString()), FText::FromString(GetBodyDebugName())));
					}
					break;
				}
			}
		}
		else if (GeomType == PxGeometryType::eTRIANGLEMESH)
		{
			PxTriangleMeshGeometry PTriMeshGeom;
			PShape->getTriangleMeshGeometry(PTriMeshGeom);
			SCENE_UNLOCK_READ(PScene);

			// Create tri-mesh shape
			if(BodySetup->TriMesh != NULL || BodySetup->TriMeshNegX != NULL)
			{
				// Please note that this one we don't inverse old scale, but just set new one
				FVector NewScale3D = inScale3D; 
				FVector Scale3DAbs(FMath::Abs(NewScale3D.X), FMath::Abs(NewScale3D.Y), FMath::Abs(NewScale3D.Z)); // magnitude of scale (sign removed)

				PxTransform PNewLocalPose;
				bool bUseNegX = CalcMeshNegScaleCompensation(NewScale3D, PNewLocalPose);

				// Only case where TriMeshNegX should be null is BSP, which should not require negX version
				if(bUseNegX && BodySetup->TriMeshNegX == NULL)
				{
					UE_LOG(LogPhysics, Warning,  TEXT("FBodyInstance::UpdateBodyScale: Want to use NegX but it doesn't exist! %s"), *BodySetup->GetPathName() );
				}

				PxTriangleMesh* UseTriMesh = bUseNegX ? BodySetup->TriMeshNegX : BodySetup->TriMesh;
				if(UseTriMesh != NULL)
				{
					PTriMeshGeom.triangleMesh = bUseNegX ? BodySetup->TriMeshNegX : BodySetup->TriMesh;
					PTriMeshGeom.scale.scale = U2PVector(Scale3DAbs);

					if(PTriMeshGeom.isValid())
					{
						SCOPED_SCENE_WRITE_LOCK(PScene);
						PShape->setGeometry(PTriMeshGeom);
						PShape->setLocalPose(PNewLocalPose);
						bSuccess = true;
					}
					else
					{
						FMessageLog("PIE").Warning(FText::Format(LOCTEXT("PhysicsInvalidScale", "Scale ''{0}'' is not valid on object '{1}'."), FText::FromString(inScale3D.ToString()), FText::FromString(GetBodyDebugName())));
					}
				}
			}
		}
		else
		{
			UE_LOG(LogPhysics, Error, TEXT("Unknown grom type."));
			SCENE_UNLOCK_READ(PScene);
		}
	}

	// if success, overwrite old Scale3D, otherwise, just don't do it. It will have invalid scale next time
	if ( bSuccess )
	{
		Scale3D = inScale3D;

		// update mass if required
		if (bUpdateMassWhenScaleChanges)
		{
			UpdateMassProperties();
		}
	}

	return bSuccess;
#else
	return false;
#endif
}

void FBodyInstance::UpdateInstanceSimulatePhysics()
{
#if WITH_PHYSX
	PxRigidDynamic* PRigidDynamic = GetPxRigidDynamic();
	if(PRigidDynamic != NULL)
	{
		// In skeletal case, we need both our bone and skelcomponent flag to be true.
		// This might be 'and'ing us with ourself, but thats fine.
		const bool bUseSimulate = IsInstanceSimulatingPhysics();

		// If we want it fixed, and it is currently not kinematic
		bool bNewKinematic = (bUseSimulate == false);
		{
			SCOPED_SCENE_WRITE_LOCK(PRigidDynamic->getScene());
			PRigidDynamic->setRigidDynamicFlag(PxRigidDynamicFlag::eKINEMATIC, bNewKinematic);
		}

		if (bUseSimulate)
		{
			PhysicsBlendWeight = 1.f;
		}
		else 
		{
			PhysicsBlendWeight = 0.f;
		}
	}
#endif
}


void FBodyInstance::SetInstanceSimulatePhysics(bool bSimulate, bool bMaintainPhysicsBlending)
{
#if WITH_PHYSX
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (bSimulate)
	{
		PxRigidActor* PRigidActor = GetPxRigidActor();
		if(PRigidActor == NULL)
		{
			FMessageLog("PIE").Warning(FText::Format(LOCTEXT("SimPhysNoBody", "Trying to simulate physics on ''{0}'' but no physics body."), 
				FText::FromString(GetPathNameSafe(OwnerComponent.Get()))));
		}
		else if(!PRigidActor->isRigidDynamic())
		{
			FMessageLog("PIE").Warning(FText::Format(LOCTEXT("SimPhysStatic", "Trying to simulate physics on ''{0}'' but it is static."), 
				FText::FromString(GetPathNameSafe(OwnerComponent.Get()))));
		}
	}
#endif
#endif // WITH_PHYSX

	// If we are enabling simulation, and we are the root body of our component, we detach the component 
	if(bSimulate == true && OwnerComponent.IsValid() && OwnerComponent->IsRegistered() && OwnerComponent->AttachParent != NULL && OwnerComponent->GetBodyInstance() == this)
	{
		OwnerComponent->DetachFromParent(true);
	}

	bSimulatePhysics = bSimulate;
	if ( !bMaintainPhysicsBlending )
	{
		if (bSimulatePhysics)
		{
			PhysicsBlendWeight = 1.f;
		}
		else
		{
			PhysicsBlendWeight = 0.f;
		}
	}

	UpdateInstanceSimulatePhysics();
}

bool FBodyInstance::IsInstanceSimulatingPhysics()
{
	// if I'm simulating or owner is simulating
	return ShouldInstanceSimulatingPhysics() && IsValidBodyInstance();
}

bool FBodyInstance::ShouldInstanceSimulatingPhysics()
{
	// if I'm simulating or owner is simulating
	if ( BodySetup.IsValid() && BodySetup.Get()->PhysicsType == PhysType_Default )
	{
		// if derive from owner, and owner is simulating, this should simulate
		if (OwnerComponent != NULL && OwnerComponent->BodyInstance.bSimulatePhysics)
		{
			return true;
		}

		// or else, it should look its own setting
	}

	return bSimulatePhysics;
}

bool FBodyInstance::IsValidBodyInstance() const
{
	bool Retval = false;
#if WITH_PHYSX
	PxRigidActor* PActor = GetPxRigidActor();
	if(PActor != NULL)
	{
		Retval = true;
	}
#endif // WITH_PHYSX

	return Retval;
}


FTransform FBodyInstance::GetUnrealWorldTransform() const
{
#if WITH_PHYSX
	PxTransform PTM = PxTransform::createIdentity();

	PxRigidActor* PActor = GetPxRigidActor();
	if(PActor != NULL)
	{		
		SCOPED_SCENE_READ_LOCK(PActor->getScene());
		PTM = PActor->getGlobalPose();
	}

	return P2UTransform(PTM);
#endif // WITH_PHYSX

	return FTransform::Identity;
}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
extern bool GShouldLogOutAFrameOfSetBodyTransform;
#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

void FBodyInstance::SetBodyTransform(const FTransform& NewTransform, bool bTeleport)
{
	SCOPE_CYCLE_COUNTER(STAT_SetBodyTransform);
	
#if WITH_PHYSX
	PxRigidActor* RigidActor = GetPxRigidActor();
	if(RigidActor == NULL)
	{
		return;
	}

	PxRigidDynamic* PRigidDynamic = GetPxRigidDynamic();

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if(GShouldLogOutAFrameOfSetBodyTransform == true)
	{
		UE_LOG(LogPhysics, Log, TEXT("SetBodyTransform: %s"), *GetBodyDebugName());
	}
#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

	// Catch NaNs and elegantly bail out.
	if( !ensureMsgf(!NewTransform.ContainsNaN(), TEXT("SetBodyTransform contains NaN (%s: %s)\n%s"), *GetPathNameSafe(OwnerComponent.Get()), *GetPathNameSafe(OwnerComponent.Get()->GetOuter()), *NewTransform.ToString()) )
	{
		return;
	}

	// Do nothing if already in correct place
	{
		SCOPED_SCENE_READ_LOCK(RigidActor->getScene());
		const PxTransform PCurrentPose = RigidActor->getGlobalPose();
		if( NewTransform.Equals(P2UTransform(PCurrentPose)) )
		{
			return;
		}
	}

	const PxTransform PNewPose = U2PTransform(NewTransform);
	check(PNewPose.isValid());

	SCENE_LOCK_WRITE(RigidActor->getScene());
	// SIMULATED & KINEMATIC
	if(PRigidDynamic)
	{
		// If kinematic and not teleporting, set kinematic target
		if(!IsRigidDynamicNonKinematic(PRigidDynamic) && !bTeleport)
		{
			const PxScene * PScene = PRigidDynamic->getScene();
			FPhysScene * PhysScene = FPhysxUserData::Get<FPhysScene>(PScene->userData);
			PhysScene->SetKinematicTarget(this, NewTransform);
		}
		// Otherwise, set global pose
		else
		{
			PRigidDynamic->setGlobalPose(PNewPose);
		}
	}
	// STATIC
	else
	{
		const bool bIsGame = !GIsEditor || (OwnerComponent != NULL && OwnerComponent->GetWorld()->IsGameWorld());
		// Do NOT move static actors in-game, give a warning but let it happen
		if( bIsGame )
		{
			const FString ComponentPathName = (OwnerComponent != NULL) ? OwnerComponent->GetPathName() : TEXT("NONE");
			UE_LOG(LogPhysics, Warning, TEXT("MoveFixedBody: Trying to move component'%s' with a non-Movable Mobility."), *ComponentPathName);
		}
		// In EDITOR, go ahead and move it with no warning, we are editing the level
		RigidActor->setGlobalPose(PNewPose);
	}

	SCENE_UNLOCK_WRITE(RigidActor->getScene());

#endif  // WITH_PHYSX
}

FVector FBodyInstance::GetUnrealWorldVelocity()
{
	FVector LinVel(0,0,0);
#if WITH_PHYSX
	PxRigidDynamic* PRigidDynamic = GetPxRigidDynamic();
	if(PRigidDynamic != NULL)
	{
		SCOPED_SCENE_READ_LOCK(PRigidDynamic->getScene());
		LinVel = P2UVector(PRigidDynamic->getLinearVelocity());
	}
#endif // WITH_PHYSX
	return LinVel;
}

/** Note: returns angular velocity in degrees per second. */
FVector FBodyInstance::GetUnrealWorldAngularVelocity()
{
	FVector AngVel(0,0,0);
#if WITH_PHYSX
	PxRigidDynamic* PRigidDynamic = GetPxRigidDynamic();
	if(PRigidDynamic != NULL)
	{
		SCOPED_SCENE_READ_LOCK(PRigidDynamic->getScene());
		AngVel = FMath::RadiansToDegrees( P2UVector(PRigidDynamic->getAngularVelocity()) );
	}
#endif // WITH_PHYSX
	return AngVel;
}

FVector FBodyInstance::GetUnrealWorldVelocityAtPoint(const FVector& Point)
{
	FVector LinVel(0,0,0);

#if WITH_PHYSX
	PxRigidDynamic* PRigidDynamic = GetPxRigidDynamic();
	if(PRigidDynamic != NULL)
	{
		SCOPED_SCENE_READ_LOCK(PRigidDynamic->getScene());
		PxVec3 PPoint = U2PVector(Point);
		LinVel = P2UVector( PxRigidBodyExt::getVelocityAtPos(*PRigidDynamic, PPoint) );
	}
#endif // WITH_PHYSX

	return LinVel;
}


FVector FBodyInstance::GetCOMPosition()
{
#if WITH_PHYSX
	PxRigidDynamic* PRigidDynamic = GetPxRigidDynamic();
	if(PRigidDynamic)
	{
		SCOPED_SCENE_READ_LOCK(PRigidDynamic->getScene());
		PxTransform PLocalCOM = PRigidDynamic->getCMassLocalPose();
		PxVec3 PWorldCOM = PRigidDynamic->getGlobalPose().transform(PLocalCOM.p);
		return P2UVector(PWorldCOM);
	}
	else
	{
		return FVector::ZeroVector;
	}
#else
	return FVector::ZeroVector;
#endif // WITH_PHYSX
}

float FBodyInstance::GetBodyMass() const
{
	float Retval = 0.f;

#if WITH_PHYSX
	PxRigidDynamic* PRigidDynamic = GetPxRigidDynamic();
	if(PRigidDynamic)
	{
		SCOPED_SCENE_READ_LOCK(PRigidDynamic->getScene());
		Retval = PRigidDynamic->getMass();
	}
	else
	{
		Retval = 0.0f;
	}
#endif // WITH_PHYSX

	return Retval;
}

FBox FBodyInstance::GetBodyBounds() const
{
	FBox Bounds;

#if WITH_PHYSX
	PxRigidDynamic* PRigidDynamic = GetPxRigidDynamic();
	if(PRigidDynamic)
	{
		SCOPED_SCENE_READ_LOCK(PRigidDynamic->getScene());
		
		PxBounds3 PBounds = PRigidDynamic->getWorldBounds();

		Bounds.Min = P2UVector(PBounds.minimum);
		Bounds.Max = P2UVector(PBounds.maximum);
	}
#endif // WITH_PHYSX

	return Bounds;
}

void FBodyInstance::DrawCOMPosition( FPrimitiveDrawInterface* PDI, float COMRenderSize, const FColor& COMRenderColor )
{
	if(IsValidBodyInstance())
	{
		DrawWireStar(PDI, GetCOMPosition(), COMRenderSize, COMRenderColor, SDPG_World);
	}
}

/** Utility for copying properties from one BodyInstance to another. */
void FBodyInstance::CopyBodyInstancePropertiesFrom(const FBodyInstance* FromInst)
{
	// No copying of runtime instances (strictly defaults off BodySetup)
	check(FromInst);
	check(FromInst->OwnerComponent.Get() == NULL);
	check(FromInst->BodySetup.Get() == NULL);
#if WITH_PHYSX
	check(!FromInst->RigidActorSync);
	check(!FromInst->RigidActorAsync);
	check(!FromInst->BodyAggregate);
#endif //WITH_PHYSX
	check(FromInst->SceneIndexSync == 0);
	check(FromInst->SceneIndexAsync == 0);

	//check(!OwnerComponent);
#if WITH_PHYSX
	check(!RigidActorSync);
	check(!RigidActorAsync);
	check(!BodyAggregate);
#endif //WITH_PHYSX
	//check(SceneIndex == 0);

	*this = *FromInst;
}


#if WITH_PHYSX
PxRigidActor* FBodyInstance::GetPxRigidActor(int32 SceneType) const
{
	// Negative scene type means to return whichever is not NULL, preferring the sync scene.
	if( SceneType < 0 )
	{
		return RigidActorSync != NULL ? RigidActorSync : RigidActorAsync;
	}
	else
	// Otherwise return the specified actor
	if( SceneType < PST_MAX )
	{
		return SceneType == PST_Sync ? RigidActorSync : RigidActorAsync;
	}
	return NULL;
}

PxRigidDynamic* FBodyInstance::GetPxRigidDynamic() const
{
	// The logic below works because dynamic actors are non-NULL in only one scene.
	// If this assumption changes, the logic needs to change.
	if(RigidActorSync != NULL)
	{
		return RigidActorSync->isRigidDynamic();
	}
	else if(RigidActorAsync != NULL)
	{
		return RigidActorAsync->isRigidDynamic();
	}
	else
	{
		return NULL;
	}
}
#endif // WITH_PHYSX


const FWalkableSlopeOverride& FBodyInstance::GetWalkableSlopeOverride() const
{
	if (bOverrideWalkableSlopeOnInstance || !BodySetup.IsValid())
	{
		return WalkableSlopeOverride;
	}
	else
	{
		return BodySetup->WalkableSlopeOverride;
	}
}


/** 
*	Changes the current PhysMaterialOverride for this body. 
*	Note that if physics is already running on this component, this will _not_ alter its mass/inertia etc, it will only change its 
*	surface properties like friction and the damping.
*/
void FBodyInstance::SetPhysMaterialOverride( UPhysicalMaterial* NewPhysMaterial )
{
	// Save ref to PhysicalMaterial
	PhysMaterialOverride = NewPhysMaterial;

	// Go through the chain of physical materials and update the NxActor
	UpdatePhysicalMaterials();
}




UPhysicalMaterial* FBodyInstance::GetSimplePhysicalMaterial()
{
	check( GEngine->DefaultPhysMaterial != NULL );

	// Find the PhysicalMaterial we need to apply to the physics bodies.
	// (LOW priority) Engine Mat, Material PhysMat, BodySetup Mat, Component Override, Body Override (HIGH priority)

	UPhysicalMaterial * ReturnPhysMaterial = NULL;

	// BodyInstance override
	if( PhysMaterialOverride != NULL)	
	{
		ReturnPhysMaterial = PhysMaterialOverride;
		check(!ReturnPhysMaterial || ReturnPhysMaterial->IsValidLowLevel());
	}
	// Component override
	else if( OwnerComponent.IsValid() && OwnerComponent->BodyInstance.PhysMaterialOverride != NULL )
	{
		ReturnPhysMaterial = OwnerComponent->BodyInstance.PhysMaterialOverride;
		check(!ReturnPhysMaterial || ReturnPhysMaterial->IsValidLowLevel());
	}
	// BodySetup
	else if( BodySetup.IsValid() && BodySetup->PhysMaterial != NULL )
	{
		ReturnPhysMaterial = BodySetup->PhysMaterial;
		check(!ReturnPhysMaterial || ReturnPhysMaterial->IsValidLowLevel());
	}
	else
	{
		// See if the Material has a PhysicalMaterial
		UMeshComponent* MeshComp = Cast<UMeshComponent>(OwnerComponent.Get());
		UPhysicalMaterial* PhysMatFromMaterial = NULL;
		if (MeshComp != NULL)
		{
			UMaterialInterface* Material = MeshComp->GetMaterial(0);
			if(Material != NULL)
			{
				PhysMatFromMaterial = Material->GetPhysicalMaterial();
			}
		}

		if( PhysMatFromMaterial != NULL )
		{
			ReturnPhysMaterial = PhysMatFromMaterial;
			check(!ReturnPhysMaterial || ReturnPhysMaterial->IsValidLowLevel());
		}
		// fallback is default physical material
		else
		{
			ReturnPhysMaterial = GEngine->DefaultPhysMaterial;
			check(!ReturnPhysMaterial || ReturnPhysMaterial->IsValidLowLevel());
		}
	}

	return ReturnPhysMaterial;
}

TArray<UPhysicalMaterial*> FBodyInstance::GetComplexPhysicalMaterials()
{
	TArray<UPhysicalMaterial*> PhysMaterials;

	check( GEngine->DefaultPhysMaterial != NULL );
	// See if the Material has a PhysicalMaterial
	UPrimitiveComponent* PrimComp = OwnerComponent.Get();
	if(PrimComp)
	{
		const int32 NumMaterials = PrimComp->GetNumMaterials();
		PhysMaterials.SetNum(NumMaterials);

		for(int32 MatIdx=0; MatIdx<NumMaterials; MatIdx++)
		{
			UPhysicalMaterial* PhysMat = GEngine->DefaultPhysMaterial;
			UMaterialInterface* Material = PrimComp->GetMaterial(MatIdx);
			if(Material)
			{
				PhysMat = Material->GetPhysicalMaterial();
			}

			check(PhysMat != NULL);
			PhysMaterials[MatIdx] = PhysMat;
		}
	}

	return PhysMaterials;
}

#if WITH_PHYSX
/** Util for finding the number of 'collision sim' shapes on this PxRigidActor */
int32 GetNumSimShapes(PxRigidDynamic* PRigidDynamic)
{
	TArray<PxShape*> PShapes;
	PShapes.AddZeroed(PRigidDynamic->getNbShapes());
	int32 NumShapes = PRigidDynamic->getShapes(PShapes.GetData(), PShapes.Num());

	int32 NumSimShapes = 0;

	for(int32 ShapeIdx=0; ShapeIdx<NumShapes; ShapeIdx++)
	{
		PxShape* PShape = PShapes[ShapeIdx];
		if(PShape->getFlags() & PxShapeFlag::eSIMULATION_SHAPE)
		{
			NumSimShapes++;
		}
	}

	return NumSimShapes;
}
#endif // WITH_PHYSX


void FBodyInstance::UpdateMassProperties()
{
#if WITH_PHYSX
	check(BodySetup != NULL);
	PxRigidDynamic* PRigidDynamic = GetPxRigidDynamic();
	if((PRigidDynamic != NULL) && (GetNumSimShapes(PRigidDynamic) > 0))
	{
		// First, reset mass to default
		UPhysicalMaterial* PhysMat  = GetSimplePhysicalMaterial();

		// physical material - nothing can weigh less than hydrogen (0.09 kg/m^3)
		float DensityKGPerCubicUU = FMath::Max(0.00009f, PhysMat->Density * 0.001f);
		PxRigidBodyExt::updateMassAndInertia(*PRigidDynamic, DensityKGPerCubicUU);

		// Then scale mass to avoid big differences between big and small objects.
		float OldMass = PRigidDynamic->getMass();

		float UsePow = FMath::Clamp<float>(PhysMat->RaiseMassToPower, KINDA_SMALL_NUMBER, 1.f);
		float NewMass = FMath::Pow(OldMass, UsePow);

		// Apply user-defined mass scaling.
		NewMass *= FMath::Clamp<float>(MassScale, 0.01f, 100.0f);

		//UE_LOG(LogPhysics, Log,  TEXT("OldMass: %f NewMass: %f"), OldMass, NewMass );

		check (NewMass > 0.f);

		float MassRatio = NewMass/OldMass;
		PxVec3 InertiaTensor = PRigidDynamic->getMassSpaceInertiaTensor();
		PRigidDynamic->setMassSpaceInertiaTensor(InertiaTensor * MassRatio);
		PRigidDynamic->setMass(NewMass);

		// Apply the COMNudge
		if(!COMNudge.IsZero())
		{
			PxVec3 PCOMNudge = U2PVector(COMNudge);
			PxTransform PCOMTransform = PRigidDynamic->getCMassLocalPose();
			PCOMTransform.p += PCOMNudge;
			PRigidDynamic->setCMassLocalPose(PCOMTransform);
		}
	}
#endif

}

void FBodyInstance::UpdateDampingProperties()
{
#if WITH_PHYSX
	check(BodySetup != NULL);
	PxRigidDynamic* PRigidDynamic = GetPxRigidDynamic();
	if (PRigidDynamic != NULL)
	{
		SCOPED_SCENE_WRITE_LOCK(PRigidDynamic->getScene());
		PRigidDynamic->setLinearDamping(LinearDamping);
		PRigidDynamic->setAngularDamping(AngularDamping);
	}
#endif
}

bool FBodyInstance::IsInstanceAwake()
{
#if WITH_PHYSX
	PxRigidDynamic* PRigidDynamic = GetPxRigidDynamic();
	if (PRigidDynamic != NULL)
	{
		SCOPED_SCENE_READ_LOCK(PRigidDynamic->getScene());
		return !PRigidDynamic->isSleeping();
	}
#endif
	return false;
}

void FBodyInstance::WakeInstance()
{
#if WITH_PHYSX
	PxRigidDynamic* PRigidDynamic = GetPxRigidDynamic();
	if (IsRigidDynamicNonKinematic(PRigidDynamic))
	{
		SCOPED_SCENE_WRITE_LOCK(PRigidDynamic->getScene());
		PRigidDynamic->wakeUp();
	}
#endif
}

void FBodyInstance::PutInstanceToSleep()
{
#if WITH_PHYSX
	PxRigidDynamic* PRigidDynamic = GetPxRigidDynamic();
	if (IsRigidDynamicNonKinematic(PRigidDynamic))
	{
		PRigidDynamic->putToSleep();
	}
#endif //WITH_PHYSX
}

void FBodyInstance::SetLinearVelocity(const FVector& NewVel, bool bAddToCurrent)
{
#if WITH_PHYSX
	PxRigidDynamic* PRigidDynamic = GetPxRigidDynamic();
	if (PRigidDynamic != NULL)
	{
		PxVec3 PNewVel = U2PVector(NewVel);

		if(bAddToCurrent)
		{
			PxVec3 POldVel = PRigidDynamic->getLinearVelocity();
			PNewVel += POldVel;
		}

		PRigidDynamic->setLinearVelocity(PNewVel);
	}
#endif // WITH_PHYSX
}

/** Note NewAngVel is in degrees per second */
void FBodyInstance::SetAngularVelocity(const FVector& NewAngVel, bool bAddToCurrent)
{
#if WITH_PHYSX
	PxRigidDynamic* PRigidDynamic = GetPxRigidDynamic();
	if (PRigidDynamic != NULL)
	{
		PxVec3 PNewAngVel = U2PVector( FMath::DegreesToRadians(NewAngVel) );

		if(bAddToCurrent)
		{
			PxVec3 POldAngVel = PRigidDynamic->getAngularVelocity();
			PNewAngVel += POldAngVel;
		}

		PRigidDynamic->setAngularVelocity(PNewAngVel);
	}
#endif //WITH_PHYSX
}



void FBodyInstance::AddForce(const FVector& Force)
{
#if WITH_PHYSX
	PxRigidDynamic* PRigidDynamic = GetPxRigidDynamic();
	if(IsRigidDynamicNonKinematic(PRigidDynamic))
	{
		const PxScene * PScene = PRigidDynamic->getScene();
		FPhysScene * PhysScene = FPhysxUserData::Get<FPhysScene>(PScene->userData);
		PhysScene->AddForce(this, Force);
		
	}
#endif // WITH_PHYSX
}

void FBodyInstance::AddForceAtPosition(const FVector& Force, const FVector& Position)
{
#if WITH_PHYSX
	PxRigidDynamic* PRigidDynamic = GetPxRigidDynamic();
	if(IsRigidDynamicNonKinematic(PRigidDynamic))
	{
		const PxScene * PScene = PRigidDynamic->getScene();
		FPhysScene * PhysScene = FPhysxUserData::Get<FPhysScene>(PScene->userData);
		PhysScene->AddForceAtPosition(this, Force, Position);
	}
#endif // WITH_PHYSX
}

void FBodyInstance::AddTorque(const FVector& Torque)
{
#if WITH_PHYSX
	PxRigidDynamic* PRigidDynamic = GetPxRigidDynamic();
	if(IsRigidDynamicNonKinematic(PRigidDynamic))
	{
		const PxScene * PScene = PRigidDynamic->getScene();
		FPhysScene * PhysScene = FPhysxUserData::Get<FPhysScene>(PScene->userData);
		PhysScene->AddTorque(this, Torque);
	}
#endif // WITH_PHYSX
}

void FBodyInstance::AddImpulse(const FVector& Impulse, bool bVelChange)
{
#if WITH_PHYSX
	PxRigidDynamic* PRigidDynamic = GetPxRigidDynamic();
	if(IsRigidDynamicNonKinematic(PRigidDynamic))
	{
		PxForceMode::Enum Mode = bVelChange ? PxForceMode::eVELOCITY_CHANGE : PxForceMode::eIMPULSE;
		PRigidDynamic->addForce(U2PVector(Impulse), Mode, true);
	}
#endif // WITH_PHYSX
}

void FBodyInstance::AddImpulseAtPosition(const FVector& Impulse, const FVector& Position)
{
#if WITH_PHYSX
	PxRigidDynamic* PRigidDynamic = GetPxRigidDynamic();
	if(IsRigidDynamicNonKinematic(PRigidDynamic))
	{
		PxForceMode::Enum Mode = PxForceMode::eIMPULSE; // does not support eVELOCITY_CHANGE
		PxRigidBodyExt::addForceAtPos(*PRigidDynamic, U2PVector(Impulse), U2PVector(Position), Mode, true);
	}
#endif // WITH_PHYSX
}

void FBodyInstance::SetInstanceNotifyRBCollision(bool bNewNotifyCollision)
{
	bNotifyRigidBodyCollision = bNewNotifyCollision;
	UpdatePhysicsFilterData();
}


void FBodyInstance::SetEnableGravity( bool bInGravityEnabled )
{
	if (bEnableGravity != bInGravityEnabled)
	{
		bEnableGravity = bInGravityEnabled;
#if WITH_PHYSX
		PxRigidDynamic* PActor = GetPxRigidDynamic();
		if (PActor != NULL)
		{
			PActor->setActorFlag( PxActorFlag::eDISABLE_GRAVITY, !bEnableGravity );
		}
#endif // WITH_PHYSX

		if (bEnableGravity)
		{
			WakeInstance();
		}
	}
}


void FBodyInstance::AddRadialImpulseToBody(const FVector& Origin, float Radius, float Strength, uint8 Falloff, bool bVelChange)
{
#if WITH_PHYSX
	PxRigidDynamic* PRigidDynamic = GetPxRigidDynamic();
	if(IsRigidDynamicNonKinematic(PRigidDynamic))
	{
		AddRadialImpulseToPxRigidDynamic(*PRigidDynamic, Origin, Radius, Strength, Falloff, bVelChange);
	}
#endif // WITH_PHYSX
}

void FBodyInstance::AddRadialForceToBody(const FVector& Origin, float Radius, float Strength, uint8 Falloff)
{
#if WITH_PHYSX
	PxRigidDynamic* PRigidDynamic = GetPxRigidDynamic();
	if(IsRigidDynamicNonKinematic(PRigidDynamic))
	{
		AddRadialForceToPxRigidDynamic(*PRigidDynamic, Origin, Radius, Strength, Falloff);
	}
#endif // WITH_PHYSX
}


FString FBodyInstance::GetBodyDebugName()
{
	FString DebugName;

	if(OwnerComponent != NULL)
	{
		DebugName = OwnerComponent->GetPathName();
	}

	if(BodySetup != NULL && BodySetup->BoneName != NAME_None)
	{
		DebugName += FString(TEXT(" ")) + BodySetup->BoneName.ToString();
	}

	return DebugName;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// COLLISION

bool FBodyInstance::LineTrace(struct FHitResult& OutHit,const FVector& Start,const FVector& End, bool bTraceComplex, bool bReturnPhysicalMaterial)
{
	SCOPE_CYCLE_COUNTER(STAT_Collision_RaycastAny);

	OutHit.TraceStart = Start;
	OutHit.TraceEnd = End;

#if WITH_PHYSX
	const PxRigidActor* RigidBody = GetPxRigidActor();
	if (RigidBody == NULL || RigidBody->getNbShapes()==0)
	{
		return false;
	}

	const FVector Delta = End - Start;
	const float DeltaMag = Delta.Size();
	if( DeltaMag > KINDA_SMALL_NUMBER )
	{
		// Create filter data used to filter collisions, should always return eTOUCH for LineTraceComponent		
		PxSceneQueryFlags POutputFlags = PxSceneQueryFlag::eIMPACT | PxSceneQueryFlag::eNORMAL | PxSceneQueryFlag::eDISTANCE;

		PxRaycastHit BestHit;
		float BestHitDistance = BIG_NUMBER;

		// Get all the shapes from the actor
		TArray<PxShape*> PShapes;
		PShapes.AddZeroed(RigidBody->getNbShapes());
		int32 NumShapes = RigidBody->getShapes(PShapes.GetData(), PShapes.Num());

		// Iterate over each shape
		for(int32 ShapeIdx=0; ShapeIdx<PShapes.Num(); ShapeIdx++)
		{
			PxShape* PShape = PShapes[ShapeIdx];
			check(PShape);

			const PxU32 HitBufferSize = 1; 
			PxRaycastHit PHits[HitBufferSize];

			// Filter so we trace against the right kind of collision
			PxFilterData ShapeFilter = PShape->getQueryFilterData();
			const bool bShapeIsComplex = (ShapeFilter.word3 & EPDF_ComplexCollision) != 0;
			const bool bShapeIsSimple = (ShapeFilter.word3 & EPDF_SimpleCollision) != 0;
			if((bTraceComplex && bShapeIsComplex) || (!bTraceComplex && bShapeIsSimple))
			{
				const int32 ArraySize = ARRAY_COUNT(PHits);
				// only care about one hit - not closest hit			
				const PxI32 NumHits = PxGeometryQuery::raycast(U2PVector(Start), U2PVector(Delta/DeltaMag), PShape->getGeometry().any(), PxShapeExt::getGlobalPose(*PShape, *RigidBody), DeltaMag, POutputFlags, ArraySize, PHits, true);


				if ( ensure (NumHits <= ArraySize) )
				{
					for(int HitIndex = 0; HitIndex < NumHits; HitIndex++)
					{
						if(PHits[HitIndex].distance < BestHitDistance)
						{
							BestHitDistance = PHits[HitIndex].distance;
							BestHit = PHits[HitIndex];

							// we don't get Shape information when we access via PShape, so I filled it up
							BestHit.shape = PShape;
							BestHit.actor = PShape->getActor();
						}
					}
				}
			}
		}

		if(BestHitDistance < BIG_NUMBER)
		{
			// we just like to make sure if the hit is made, set to test touch
			PxFilterData QueryFilter;
			QueryFilter.word2 = 0xFFFFF;

			PxTransform PStartTM(U2PVector(Start));
			ConvertQueryImpactHit(BestHit, OutHit, DeltaMag, QueryFilter, Start, End, NULL, PStartTM, false, bReturnPhysicalMaterial);
			return true;
		}
	}
#endif //WITH_PHYSX
	return false;
}

bool FBodyInstance::Sweep(struct FHitResult& OutHit, const FVector& Start, const FVector& End, const FCollisionShape& CollisionShape, bool bTraceComplex)
{
	OutHit.TraceStart = Start;
	OutHit.TraceEnd = End;

#if WITH_PHYSX
	const PxRigidActor* RigidBody = GetPxRigidActor();
	if(RigidBody == NULL || RigidBody->getNbShapes()==0 || OwnerComponent==NULL)
	{
		return false;
	}

	if(CollisionShape.IsNearlyZero())
	{
		return LineTrace(OutHit, Start, End, bTraceComplex);
	}

	switch(CollisionShape.ShapeType)
	{
	case ECollisionShape::Box:
		{
			const PxBoxGeometry PBoxGeom(U2PVector(CollisionShape.GetBox()));
			const PxQuat PGeomRot = PxQuat::createIdentity();

			return InternalSweep(OutHit, Start, End, CollisionShape, bTraceComplex, RigidBody, &PBoxGeom);
		}
	case ECollisionShape::Sphere:
		{
			PxSphereGeometry PSphereGeom(CollisionShape.GetSphereRadius());
			PxQuat PGeomRot = PxQuat::createIdentity();

			return InternalSweep(OutHit, Start, End, CollisionShape, bTraceComplex, RigidBody, &PSphereGeom);
		}

	case ECollisionShape::Capsule:
		{
			PxCapsuleGeometry PCapsuleGeom(CollisionShape.GetCapsuleRadius(), CollisionShape.GetCapsuleAxisHalfLength());
			const PxQuat PGeomRot = PxQuat::createIdentity();
			return InternalSweep(OutHit, Start, End, CollisionShape, bTraceComplex, RigidBody, &PCapsuleGeom);
		}
	}
#endif //WITH_PHYSX
	return false;
}

#if WITH_PHYSX
bool FBodyInstance::InternalSweep(struct FHitResult& OutHit, const FVector& Start, const FVector& End, const FCollisionShape & Shape, bool bTraceComplex, const PxRigidActor* RigidBody, const PxGeometry * Geometry)
{
	const FVector Delta = End - Start;
	const float DeltaMag = Delta.Size();
	if(DeltaMag > KINDA_SMALL_NUMBER)
	{
		PxSceneQueryFlags POutputFlags = PxSceneQueryFlag::eIMPACT | PxSceneQueryFlag::eNORMAL | PxSceneQueryFlag::eDISTANCE;
		
		PxQuat PGeomRot = PxQuat::createIdentity();

		PxTransform PStartTM(U2PVector(Start), PGeomRot);
		PxTransform PCompTM(U2PTransform(OwnerComponent->ComponentToWorld));

		PxVec3 PDir = U2PVector(Delta/DeltaMag);

		PxSweepHit PHit;

		// Get all the shapes from the actor
		TArray<PxShape*> PShapes;
		PShapes.AddZeroed(RigidBody->getNbShapes());
		int32 NumShapes = RigidBody->getShapes(PShapes.GetData(), PShapes.Num());

		// Iterate over each shape
		for(int32 ShapeIdx=0; ShapeIdx<PShapes.Num(); ShapeIdx++)
		{
			PxShape* PShape = PShapes[ShapeIdx];
			check(PShape);

			// Filter so we trace against the right kind of collision
			PxFilterData ShapeFilter = PShape->getQueryFilterData();
			const bool bShapeIsComplex = (ShapeFilter.word3 & EPDF_ComplexCollision) != 0;
			const bool bShapeIsSimple = (ShapeFilter.word3 & EPDF_SimpleCollision) != 0;
			if((bTraceComplex && bShapeIsComplex) || (!bTraceComplex && bShapeIsSimple))
			{
				GeometryFromShapeStorage GeomStorage;
				PxGeometry * PGeom = GetGeometryFromShape(GeomStorage, PShape, true);
				PxTransform PGlobalPose = PCompTM.transform(PShape->getLocalPose());

				if (PGeom)
				{
					if(PxGeometryQuery::sweep(PDir, DeltaMag, *Geometry, PStartTM, *PGeom, PGlobalPose, PHit, POutputFlags))
					{
						// we just like to make sure if the hit is made
						PxFilterData QueryFilter;
						QueryFilter.word2 = 0xFFFFF;

						// we don't get Shape information when we access via PShape, so I filled it up
						PHit.shape = PShape;
						PHit.actor = PShape->getActor();
						PxTransform PStartTransform(U2PVector(Start));
						ConvertQueryImpactHit(PHit, OutHit, DeltaMag, QueryFilter, Start, End, NULL, PStartTransform, false, false);
						return true;
					}
				}
			}
		}
	}
	return false;
}
#endif //WITH_PHYSX

float FBodyInstance::GetDistanceToBody(const FVector& Point, FVector& OutPointOnBody) const
{
	OutPointOnBody = Point;

#if WITH_PHYSX
	const PxRigidActor* RigidBody = GetPxRigidActor();
	if (RigidBody == NULL || RigidBody->getNbShapes()==0 || OwnerComponent==NULL)
	{
		return -1.f;
	}

	// Get all the shapes from the actor
	TArray<PxShape*> PShapes;
	PShapes.AddZeroed(RigidBody->getNbShapes());
	int32 NumShapes = RigidBody->getShapes(PShapes.GetData(), PShapes.Num());

	const PxVec3 PPoint = U2PVector(Point);
	PxTransform PCompTM(U2PTransform(OwnerComponent->ComponentToWorld));
	float MinDistanceSqr=BIG_NUMBER;
	bool bFoundValidBody = false;

	// Iterate over each shape
	for(int32 ShapeIdx=0; ShapeIdx<PShapes.Num(); ShapeIdx++)
	{
		PxShape* PShape = PShapes[ShapeIdx];
		check(PShape);
		PxGeometry& PGeom = PShape->getGeometry().any();
		PxTransform PGlobalPose = PCompTM.transform(PShape->getLocalPose());
		PxGeometryType::Enum GeomType = PShape->getGeometryType();
		
		if (GeomType == PxGeometryType::eTRIANGLEMESH)
		{
			// Type unsupported for this function, but some other shapes will probably work. 
			continue;
		}
		bFoundValidBody = true;

		PxVec3 PClosestPoint;
		float SqrDistance = PxGeometryQuery::pointDistance(PPoint, PGeom, PGlobalPose, &PClosestPoint);
		// distance has valid data and smaller than mindistance
		if ( SqrDistance > 0.f && MinDistanceSqr > SqrDistance )
		{
			MinDistanceSqr = SqrDistance;
			OutPointOnBody = P2UVector(PClosestPoint);
		}
		else if ( SqrDistance == 0.f )
		{
			MinDistanceSqr = 0.f;
			break;
		}
	}

	if (!bFoundValidBody)
	{
		UE_LOG(LogPhysics, Warning, TEXT("GetDistanceToBody: Component (%s) has no simple collision and cannot be queried for closest point."), OwnerComponent.Get() ? *OwnerComponent->GetPathName() : TEXT("NONE"));
	}

	return (bFoundValidBody ? FMath::Sqrt(MinDistanceSqr) : -1.f);
#else
	// returns -1 if shape is not found
	return -1.f;
#endif //WITH_PHYSX
}

#if WITH_PHYSX
bool FBodyInstance::Overlap(const PxGeometry& PGeom, const PxTransform&  ShapePose)
{
	const PxRigidActor* RigidBody = GetPxRigidActor();

	if (RigidBody==NULL || RigidBody->getNbShapes()==0)
	{
		return false;
	}

	// Get all the shapes from the actor
	TArray<PxShape*> PShapes;
	PShapes.AddZeroed(RigidBody->getNbShapes());
	int32 NumShapes = RigidBody->getShapes(PShapes.GetData(), PShapes.Num());

	// Iterate over each shape
	for(int32 ShapeIdx=0; ShapeIdx<PShapes.Num(); ++ShapeIdx)
	{
		const PxShape* PShape = PShapes[ShapeIdx];
		check(PShape);
		
		if(PxGeometryQuery::overlap(PShape->getGeometry().any(), PxShapeExt::getGlobalPose(*PShape, *RigidBody), PGeom, ShapePose))
		{
			return true;
		}
	}
	return false;
}
#endif //WITH_PHYSX


void FBodyInstance::LoadProfileData(bool bVerifyProfile)
{
	if ( bVerifyProfile )
	{
		// if collision profile name exists, 
		// check with current settings
		// if same, then keep the profile name
		// if not same, that means it has been modified from default
		// leave it as it is, and clear profile name
		if ( CollisionProfileName != NAME_None )
		{
			FCollisionResponseTemplate Template;
			if ( UCollisionProfile::Get()->GetProfileTemplate(CollisionProfileName, Template) ) 
			{
				// this function is only used for old code that did require verification of using profile or not
				// so that means it will have valid ResponsetoChannels value, so this is okay to access. 
				if (Template.IsEqual(CollisionEnabled, ObjectType, CollisionResponses.GetResponseContainer()) == false)
				{
					CollisionProfileName = NAME_None;
				}
			}
			else
			{
				UE_LOG(LogPhysics, Warning, TEXT("COLLISION PROFILE [%s] is not found"), *CollisionProfileName.ToString());
				// if not nothing to do
				CollisionProfileName = NAME_None;
			}
		}

	}
	else
	{
		if ( CollisionProfileName != NAME_None )
		{
			if ( UCollisionProfile::Get()->ReadConfig(CollisionProfileName, *this) == false)
			{
				// clear the name
				CollisionProfileName = NAME_None;
			}
		}

		// no profile, so it just needs to update container from array data
		if ( CollisionProfileName == NAME_None)
		{
			CollisionResponses.UpdateResponseContainerFromArray();
		}
	}
}

SIZE_T FBodyInstance::GetBodyInstanceResourceSize(EResourceSizeMode::Type Mode) const
{
	SIZE_T ResSize = 0;

#if WITH_PHYSX
	// Get size of PhysX data, skipping contents of 'shared data'
	if (RigidActorSync != NULL)
	{
		ResSize += GetPhysxObjectSize(RigidActorSync, FPhysxSharedData::Get().GetCollection());
	}

	if (RigidActorAsync != NULL)
	{
		ResSize += GetPhysxObjectSize(RigidActorAsync, FPhysxSharedData::Get().GetCollection());
	}
#endif

	return ResSize;
}

void FBodyInstance::FixupData(class UObject * Loader)
{
	check (Loader);

	int32 const UE4Version = Loader->GetLinkerUE4Version();
	if(UE4Version < VER_UE4_CHANGE_BENABLECOLLISION_TO_COLLISIONENABLED)
	{
		UpdateFromDeprecatedEnableCollision();
	}

	if (UE4Version < VER_UE4_SAVE_COLLISIONRESPONSE_PER_CHANNEL)
	{
		CollisionResponses.SetCollisionResponseContainer(ResponseToChannels_DEPRECATED);
	}

	// Load profile. If older version, please verify profile name first
	bool bNeedToVerifyProfile = (UE4Version < VER_UE4_COLLISION_PROFILE_SETTING) || 
		// or shape component needs to convert since we added profile
		(UE4Version < VER_UE4_SAVE_COLLISIONRESPONSE_PER_CHANNEL && Loader->IsA(UShapeComponent::StaticClass()));
	LoadProfileData(bNeedToVerifyProfile);

	// if profile isn't set, then fix up channel responses
	if( CollisionProfileName == NAME_None )
	{
		if (UE4Version >= VER_UE4_SAVE_COLLISIONRESPONSE_PER_CHANNEL)
		{
			CollisionResponses.UpdateResponseContainerFromArray();
		}
	}
}

bool FBodyInstance::UseAsyncScene() const
{
	bool bHasAsyncScene = true;
	if (OwnerComponent.IsValid())
	{
		bHasAsyncScene = OwnerComponent->GetWorld()->GetPhysicsScene()->HasAsyncScene();
	}
	return bUseAsyncScene && bHasAsyncScene;
}

#undef LOCTEXT_NAMESPACE
