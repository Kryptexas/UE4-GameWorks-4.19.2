// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	BodySetup.cpp
=============================================================================*/ 

#include "EnginePrivate.h"
#include "TargetPlatform.h"

#if WITH_PHYSX
	#include "PhysXSupport.h"
#endif // WITH_PHYSX


#include "PhysDerivedData.h"

#if WITH_PHYSX
	namespace
	{
		// Quaternion that converts Sphyls from UE space to PhysX space (negate Y, swap X & Z)
		// This is equivalent to a 180 degree rotation around the normalized (1, 0, 1) axis
		static const PxQuat U2PSphylBasis( PI, PxVec3( 1.0f / FMath::Sqrt( 2.0f ), 0.0f, 1.0f / FMath::Sqrt( 2.0f ) ) );
	}
#endif // WITH_PHYSX

DEFINE_LOG_CATEGORY(LogPhysics);
UBodySetup::UBodySetup(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bConsiderForBounds = true;
	bMeshCollideAll = false;
	CollisionTraceFlag = CTF_UseDefault;
	bHasCookedCollisionData = true;
	bGenerateMirroredCollision = true;
	bGenerateNonMirroredCollision = true;
	DefaultInstance.SetObjectType(ECC_PhysicsBody);
	BuildScale = 1.0f;
}

void UBodySetup::CopyBodyPropertiesFrom(class UBodySetup* FromSetup)
{
	AggGeom = FromSetup->AggGeom;

	// clear pointers copied from other BodySetup, as 
	for (int32 i = 0; i < AggGeom.ConvexElems.Num(); i++)
	{
		FKConvexElem& ConvexElem = AggGeom.ConvexElems[i];
		ConvexElem.ConvexMesh = NULL;
		ConvexElem.ConvexMeshNegX = NULL;
	}

	DefaultInstance.CopyBodyInstancePropertiesFrom(&FromSetup->DefaultInstance);
	PhysMaterial = FromSetup->PhysMaterial;
	PhysicsType = FromSetup->PhysicsType;
}

void UBodySetup::AddCollisionFrom(class UBodySetup* FromSetup)
{
	// Add shapes from static mesh
	AggGeom.SphereElems.Append(FromSetup->AggGeom.SphereElems);
	AggGeom.BoxElems.Append(FromSetup->AggGeom.BoxElems);
	AggGeom.SphylElems.Append(FromSetup->AggGeom.SphylElems);

	// Remember how many convex we already have
	int32 FirstNewConvexIdx = AggGeom.ConvexElems.Num();
	// copy convex
	AggGeom.ConvexElems.Append(FromSetup->AggGeom.ConvexElems);
	// clear pointers on convex elements
	for (int32 i = FirstNewConvexIdx; i < AggGeom.ConvexElems.Num(); i++)
	{
		FKConvexElem& ConvexElem = AggGeom.ConvexElems[i];
		ConvexElem.ConvexMesh = NULL;
		ConvexElem.ConvexMeshNegX = NULL;
	}
}


void UBodySetup::CreatePhysicsMeshes()
{
#if WITH_PHYSX
	// Create meshes from cooked data if not already done
	if(bCreatedPhysicsMeshes)
	{
		return;
	}

	// Find or create cooked physics data
	static FName PhysicsFormatName(FPlatformProperties::GetPhysicsFormat());
	FByteBulkData* FormatData = GetCookedData(PhysicsFormatName);
	if( FormatData )
	{
		if (FormatData->IsLocked())
		{
			// seems it's being already processed
			return;
		}

		ClearPhysicsMeshes(); // Make sure all are cleared before we start

		// Create physics objects
		FPhysXFormatDataReader CookedDataReader( *FormatData );
		check( !bGenerateNonMirroredCollision || CookedDataReader.ConvexMeshes.Num() == 0 || CookedDataReader.ConvexMeshes.Num() == AggGeom.ConvexElems.Num() );
		check( !bGenerateMirroredCollision || CookedDataReader.ConvexMeshesNegX.Num() == 0 || CookedDataReader.ConvexMeshesNegX.Num() == AggGeom.ConvexElems.Num() );

		//If the cooked data no longer has convex meshes, make sure to empty AggGeom.ConvexElems - otherwise we leave NULLS which cause issues, and we also read past the end of CookedDataReader.ConvexMeshes
		if( (bGenerateNonMirroredCollision && CookedDataReader.ConvexMeshes.Num() == 0) || (bGenerateMirroredCollision && CookedDataReader.ConvexMeshesNegX.Num() == 0))
		{
			AggGeom.ConvexElems.Empty();
		}

		for( int32 ElementIndex = 0; ElementIndex < AggGeom.ConvexElems.Num(); ElementIndex++ )
		{
			FKConvexElem& ConvexElem = AggGeom.ConvexElems[ElementIndex];

			if(bGenerateNonMirroredCollision)
			{
				ConvexElem.ConvexMesh = CookedDataReader.ConvexMeshes[ ElementIndex ];
				FPhysxSharedData::Get().Add(ConvexElem.ConvexMesh);
			}

			if(bGenerateMirroredCollision)
			{
				ConvexElem.ConvexMeshNegX = CookedDataReader.ConvexMeshesNegX[ ElementIndex ];
				FPhysxSharedData::Get().Add(ConvexElem.ConvexMeshNegX);
			}
		}

		if(bGenerateNonMirroredCollision)
		{
			TriMesh = CookedDataReader.TriMesh;
			FPhysxSharedData::Get().Add(TriMesh);
		}

		if(bGenerateMirroredCollision)
		{
			TriMeshNegX = CookedDataReader.TriMeshNegX;
			FPhysxSharedData::Get().Add(TriMeshNegX);
		}
	}
	else
	{
		ClearPhysicsMeshes(); // Make sure all are cleared then
	}

	// Clear the cooked data
	if (!GIsEditor)
	{
		CookedFormatData.FlushData();
	}

	bCreatedPhysicsMeshes = true;
#endif
}


void UBodySetup::ClearPhysicsMeshes()
{
#if WITH_PHYSX
	for(int32 i=0; i<AggGeom.ConvexElems.Num(); i++)
	{
		FKConvexElem* ConvexElem = &(AggGeom.ConvexElems[i]);

		if(ConvexElem->ConvexMesh != NULL)
		{
			// put in list for deferred release
			GPhysXPendingKillConvex.Add(ConvexElem->ConvexMesh);
			FPhysxSharedData::Get().Remove(ConvexElem->ConvexMesh);
			ConvexElem->ConvexMesh = NULL;
		}

		if(ConvexElem->ConvexMeshNegX != NULL)
		{
			// put in list for deferred release
			GPhysXPendingKillConvex.Add(ConvexElem->ConvexMeshNegX);
			FPhysxSharedData::Get().Remove(ConvexElem->ConvexMeshNegX);
			ConvexElem->ConvexMeshNegX = NULL;
		}
	}

	if(TriMesh != NULL)
	{
		GPhysXPendingKillTriMesh.Add(TriMesh);
		FPhysxSharedData::Get().Remove(TriMesh);
		TriMesh = NULL;
	}

	if(TriMeshNegX != NULL)
	{
		GPhysXPendingKillTriMesh.Add(TriMeshNegX);
		FPhysxSharedData::Get().Remove(TriMeshNegX);
		TriMeshNegX = NULL;
	}

	bCreatedPhysicsMeshes = false;
#endif
}

#if WITH_PHYSX
/** Util to determine whether to use NegX version of mesh, and what transform (rotation) to apply. */
bool CalcMeshNegScaleCompensation(const FVector& InScale3D, PxTransform& POutTransform)
{
	POutTransform = PxTransform::createIdentity();

	if(InScale3D.Y > 0.f)
	{
		if(InScale3D.Z > 0.f)
		{
			// no rotation needed
		}
		else
		{
			// y pos, z neg
			POutTransform.q = PxQuat(PxPi, PxVec3(0,1,0));
		}
	}
	else
	{
		if(InScale3D.Z > 0.f)
		{
			// y neg, z pos
			POutTransform.q = PxQuat(PxPi, PxVec3(0,0,1));
		}
		else
		{
			// y neg, z neg
			POutTransform.q = PxQuat(PxPi, PxVec3(1,0,0));
		}
	}

	// Use inverted mesh if determinant is negative
	return (InScale3D.X * InScale3D.Y * InScale3D.Z) < 0.f;
}

void UBodySetup::AddShapesToRigidActor(PxRigidActor* PDestActor, FVector& Scale3D)
{
#if WITH_EDITOR
	// in editor, there are a lot of things relying on body setup to create physics meshes
	CreatePhysicsMeshes();
#endif

	// if almost zero, set min scale
	// @todo fixme
	if (Scale3D.IsNearlyZero())
	{
		// set min scale
		Scale3D = FVector(0.1f);
	}
	// Determine if applied scaling is uniform. If it isn't, only convex geometry will be copied over
	float MinScale = Scale3D.GetMin(); // Uniform scaling factor
	float MinScaleAbs = FMath::Abs(MinScale);
	if ( FMath::IsNearlyZero(MinScale) )
	{
		// only one of them can be 0, we make sure they have mini set up correctly
		MinScale = 0.1f;
		MinScaleAbs = 0.1f;
	}

	FVector Scale3DAbs(FMath::Abs(Scale3D.X), FMath::Abs(Scale3D.Y), FMath::Abs(Scale3D.Z)); // magnitude of scale (sign removed)

	// Is the target a static actor
	bool bDestStatic = (PDestActor->isRigidStatic() != NULL);

	// Get contact offset params
	static const auto CVarContactOffsetFactor = IConsoleManager::Get().FindTConsoleVariableDataFloat(TEXT("p.ContactOffsetFactor"));
	const float ContactOffsetFactor = CVarContactOffsetFactor->GetValueOnGameThread();
	static const auto CVarMaxContactOffset = IConsoleManager::Get().FindTConsoleVariableDataFloat(TEXT("p.MaxContactOffset"));
	const float MaxContactOffset = CVarMaxContactOffset->GetValueOnGameThread();

	check(GEngine->DefaultPhysMaterial != NULL);
	PxMaterial* PDefaultMat = GEngine->DefaultPhysMaterial->GetPhysXMaterial();

	// Create shapes for simple collision if we do not want to use the complex collision mesh 
	// for simple queries as well
	if (CollisionTraceFlag != ECollisionTraceFlag::CTF_UseComplexAsSimple)
	{
		// Sphere primitives
		for (int32 i = 0; i < AggGeom.SphereElems.Num(); i++)
		{
			FKSphereElem* SphereElem = &AggGeom.SphereElems[i];

			PxSphereGeometry PSphereGeom;
			PSphereGeom.radius = (SphereElem->Radius * MinScaleAbs);

			if(PSphereGeom.isValid())
			{
				PxTransform PLocalPose( U2PVector( SphereElem->Center ) );
				PLocalPose.p *= MinScale;

				ensure(PLocalPose.isValid());
				PxShape* NewShape = PDestActor->createShape(PSphereGeom, *PDefaultMat, PLocalPose);

				const float ContactOffset = FMath::Min(MaxContactOffset, ContactOffsetFactor * PSphereGeom.radius);
				NewShape->setContactOffset(ContactOffset);
			}
			else
			{
				UE_LOG(LogPhysics, Log, TEXT("AddShapesToRigidActor: [%s] SphereElem[%d] invalid"), *GetPathNameSafe(GetOuter()), i);
			}
		}

		// Box primitives
		for (int32 i = 0; i < AggGeom.BoxElems.Num(); i++)
		{
			FKBoxElem* BoxElem = &AggGeom.BoxElems[i];

			PxBoxGeometry PBoxGeom;
			PBoxGeom.halfExtents.x = (0.5f * BoxElem->X * MinScaleAbs);
			PBoxGeom.halfExtents.y = (0.5f * BoxElem->Y * MinScaleAbs);
			PBoxGeom.halfExtents.z = (0.5f * BoxElem->Z * MinScaleAbs);

			FTransform BoxTransform = BoxElem->GetTransform();
			if(PBoxGeom.isValid() && BoxTransform.IsValid() )
			{
				PxTransform PLocalPose( U2PTransform( BoxTransform ));
				PLocalPose.p *= MinScale;

				ensure(PLocalPose.isValid());
				PxShape* NewShape = PDestActor->createShape(PBoxGeom, *PDefaultMat, PLocalPose);

				const float ContactOffset = FMath::Min(MaxContactOffset, ContactOffsetFactor * PBoxGeom.halfExtents.minElement());
				NewShape->setContactOffset(ContactOffset);
			}
			else
			{
				UE_LOG(LogPhysics, Log, TEXT("AddShapesToRigidActor: [%s] BoxElems[%d] invalid or has invalid transform"), *GetPathNameSafe(GetOuter()), i);
			}
		}

		// Sphyl (aka Capsule) primitives
		for (int32 i =0; i < AggGeom.SphylElems.Num(); i++)
		{
			FKSphylElem* SphylElem = &AggGeom.SphylElems[i];

			PxCapsuleGeometry PCapsuleGeom;
			PCapsuleGeom.halfHeight = (0.5f * SphylElem->Length * MinScaleAbs);
			PCapsuleGeom.radius = (SphylElem->Radius * MinScaleAbs);

			if(PCapsuleGeom.isValid())
			{
				// The stored sphyl transform assumes the sphyl axis is down Z. In PhysX, it points down X, so we twiddle the matrix a bit here (swap X and Z and negate Y).
				PxTransform PLocalPose( U2PVector( SphylElem->Center ), U2PQuat( SphylElem->Orientation ) * U2PSphylBasis );
				PLocalPose.p *= MinScale;

				ensure(PLocalPose.isValid());
				PxShape* NewShape = PDestActor->createShape(PCapsuleGeom, *PDefaultMat, PLocalPose);

				const float ContactOffset = FMath::Min(MaxContactOffset, ContactOffsetFactor * PCapsuleGeom.radius);
				NewShape->setContactOffset(ContactOffset);
			}
			else
			{
				UE_LOG(LogPhysics, Log, TEXT("AddShapesToRigidActor: [%s] SphylElems[%d] invalid"), *GetPathNameSafe(GetOuter()), i);
			}
		}

		// Create convex shapes
		for(int32 i=0; i<AggGeom.ConvexElems.Num(); i++)
		{
			FKConvexElem* ConvexElem = &(AggGeom.ConvexElems[i]);

			if(ConvexElem->ConvexMesh != NULL)
			{
				PxTransform PLocalPose;
				bool bUseNegX = CalcMeshNegScaleCompensation(Scale3D, PLocalPose);

				PxConvexMeshGeometry PConvexGeom;
				PConvexGeom.convexMesh = bUseNegX ? ConvexElem->ConvexMeshNegX : ConvexElem->ConvexMesh;
				PConvexGeom.scale.scale = U2PVector(Scale3DAbs * ConvexElem->GetTransform().GetScale3D().GetAbs());
				FTransform ConvexTransform = ConvexElem->GetTransform();
				if ( ConvexTransform.IsValid() )
				{
					PxTransform PElementTransform = U2PTransform(ConvexTransform);
					PLocalPose.q *= PElementTransform.q;
					PLocalPose.p += PElementTransform.p;

					if(PConvexGeom.isValid())
					{
						PxVec3 PBoundsExtents = PConvexGeom.convexMesh->getLocalBounds().getExtents();

						ensure(PLocalPose.isValid());
						PxShape* NewShape = PDestActor->createShape(PConvexGeom, *PDefaultMat, PLocalPose);

						const float ContactOffset = FMath::Min(MaxContactOffset, ContactOffsetFactor * PBoundsExtents.minElement());
						NewShape->setContactOffset(ContactOffset);
					}
					else
					{
						UE_LOG(LogPhysics, Log, TEXT("AddShapesToRigidActor: [%s] ConvexElem[%d] invalid"), *GetPathNameSafe(GetOuter()), i);
					}
				}
				else
				{
					UE_LOG(LogPhysics, Log, TEXT("AddShapesToRigidActor: [%s] ConvexElem[%d] has invalid transform"), *GetPathNameSafe(GetOuter()), i);
				}				
			}
			else
			{
				UE_LOG(LogPhysics, Warning, TEXT("AddShapesToRigidActor: ConvexElem is missing ConvexMesh (%d: %s)"), i, *GetPathName());
			}
		}
	}


	// Create tri-mesh shape, when we are not using simple collision shapes for 
	// complex queries as well
	if( CollisionTraceFlag != ECollisionTraceFlag::CTF_UseSimpleAsComplex && 
	    (TriMesh != NULL || TriMeshNegX != NULL) )
	{
		PxTransform PLocalPose;
		bool bUseNegX = CalcMeshNegScaleCompensation(Scale3D, PLocalPose);

		// Only case where TriMeshNegX should be null is BSP, which should not require negX version
		if(bUseNegX && TriMeshNegX == NULL)
		{
			UE_LOG(LogPhysics, Log,  TEXT("AddShapesToRigidActor: Want to use NegX but it doesn't exist! %s"), *GetPathName() );
		}

		PxTriangleMesh* UseTriMesh = bUseNegX ? TriMeshNegX : TriMesh;
		if(UseTriMesh != NULL)
		{
			PxTriangleMeshGeometry PTriMeshGeom;
			PTriMeshGeom.triangleMesh = bUseNegX ? TriMeshNegX : TriMesh;
			PTriMeshGeom.scale.scale = U2PVector(Scale3DAbs);

			if(PTriMeshGeom.isValid())
			{
				ensure(PLocalPose.isValid());

				// Create without 'sim shape' flag, problematic if it's kinematic, and it gets set later anyway.
				PxShape* NewShape = PDestActor->createShape(PTriMeshGeom, *PDefaultMat, PxShapeFlag::eSCENE_QUERY_SHAPE | PxShapeFlag::eVISUALIZATION);
				if(NewShape)
				{
					NewShape->setLocalPose(PLocalPose);

					const float ContactOffset = FMath::Min(MaxContactOffset, GPhysXCooking->getParams().skinWidth);
					NewShape->setContactOffset(ContactOffset);
				}
				else
				{
					UE_LOG(LogPhysics, Log, TEXT("Can't create new mesh shape in AddShapesToRigidActor"));
				}
			}
			else
			{
				UE_LOG(LogPhysics, Log, TEXT("AddShapesToRigidActor: TriMesh invalid"));
			}
		}
	}
}

#endif // WITH_PHYSX


#if WITH_EDITOR

void UBodySetup::RemoveSimpleCollision()
{
	AggGeom.EmptyElements();
	InvalidatePhysicsData();
}

void UBodySetup::RescaleSimpleCollision( float UniformScale )
{
	if( BuildScale != UniformScale )
	{
		float PrevBuildScale = BuildScale;
						
		// Back out the old scale when applying the new scale
		const float ScaleMultiplier = (UniformScale / PrevBuildScale);

		for (int32 i = 0; i < AggGeom.ConvexElems.Num(); i++)
		{
			FKConvexElem* ConvexElem = &(AggGeom.ConvexElems[i]);

			TArray<FVector>& Vertices = ConvexElem->VertexData;
			for (int32 VertIndex = 0; VertIndex < Vertices.Num(); ++VertIndex)
			{

				Vertices[VertIndex] *= ScaleMultiplier;
			}

			ConvexElem->UpdateElemBox();
		}

		for (int32 i = 0; i < AggGeom.SphereElems.Num(); i++)
		{
			FKSphereElem* SphereElem = &(AggGeom.SphereElems[i]);

			SphereElem->Center *= ScaleMultiplier;
			SphereElem->Radius *= ScaleMultiplier;
		}

		for (int32 i = 0; i < AggGeom.BoxElems.Num(); i++)
		{
			FKBoxElem* BoxElem = &(AggGeom.BoxElems[i]);

			BoxElem->Center *= ScaleMultiplier;
			BoxElem->X *= ScaleMultiplier;
			BoxElem->Y *= ScaleMultiplier;
			BoxElem->Z *= ScaleMultiplier;
		}


		for (int32 i = 0; i < AggGeom.SphylElems.Num(); i++)
		{
			FKSphylElem* SphylElem = &(AggGeom.SphylElems[i]);

			SphylElem->Center *= ScaleMultiplier;
			SphylElem->Radius *= ScaleMultiplier;
			SphylElem->Length *= ScaleMultiplier;
		}

		BuildScale = UniformScale;
	}
}

void UBodySetup::InvalidatePhysicsData()
{
	ClearPhysicsMeshes();
	BodySetupGuid = FGuid::NewGuid(); // change the guid
	CookedFormatData.FlushData();
}
#endif // WITH_EDITOR


void UBodySetup::BeginDestroy()
{
	Super::BeginDestroy();

	AggGeom.FreeRenderInfo();
}	

void UBodySetup::FinishDestroy()
{
	ClearPhysicsMeshes();
	Super::FinishDestroy();
}




void UBodySetup::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	// Load GUID (or create one for older versions)
	Ar << BodySetupGuid;

	// If we loaded a ZERO Guid, fix that
	if(Ar.IsLoading() && !BodySetupGuid.IsValid())
	{
		MarkPackageDirty();
		UE_LOG(LogPhysics, Log, TEXT("FIX GUID FOR: %s"), *GetPathName());
		BodySetupGuid = FGuid::NewGuid();
	}

	bool bCooked = false;
	if (Ar.UE4Ver() >= VER_UE4_ADD_COOKED_TO_BODY_SETUP)
	{
		bCooked = Ar.IsCooking();
		Ar << bCooked;
	}

	if (FPlatformProperties::RequiresCookedData() && !bCooked && Ar.IsLoading())
	{
		UE_LOG(LogPhysics, Fatal, TEXT("This platform requires cooked packages, and physX data was not cooked into %s."), *GetFullName());
	}

	if (bCooked)
	{
		if (Ar.IsCooking())
		{
			// Make sure to reset bHasCookedCollision data to true before calling GetCookedData for cooking
			bHasCookedCollisionData = true;
			FName Format = Ar.CookingTarget()->GetPhysicsFormat(this);
			bHasCookedCollisionData = GetCookedData(Format) != NULL; // Get the data from the DDC or build it

			TArray<FName> ActualFormatsToSave;
			ActualFormatsToSave.Add(Format);

			Ar << bHasCookedCollisionData;
			CookedFormatData.Serialize(Ar, this, &ActualFormatsToSave);
		}
		else
		{
			if (Ar.UE4Ver() >= VER_UE4_STORE_HASCOOKEDDATA_FOR_BODYSETUP)
			{
				Ar << bHasCookedCollisionData;
			}
			CookedFormatData.Serialize(Ar, this);
		}
	}

	if ( Ar.IsLoading() )
	{
		AggGeom.Serialize( Ar );
	}
}

void UBodySetup::PostLoad()
{
	Super::PostLoad();

	// Our owner needs to be post-loaded before us else they may not have loaded
	// their data yet.
	UObject* Outer = GetOuter();
	if (Outer)
	{
		Outer->ConditionalPostLoad();
	}

	DefaultInstance.FixupData(this);

	if ( GetLinkerUE4Version() < VER_UE4_REFACTOR_PHYSICS_BLENDING )
	{
		if ( bAlwaysFullAnimWeight_DEPRECATED )
		{
			PhysicsType = PhysType_Unfixed;
		}
		else if ( DefaultInstance.bSimulatePhysics == false )
		{
			PhysicsType = PhysType_Fixed;
		}
		else
		{
			PhysicsType = PhysType_Default;
		}
	}

	if ( GetLinkerUE4Version() < VER_UE4_BODYSETUP_COLLISION_CONVERSION )
	{
		if ( DefaultInstance.GetCollisionEnabled() == ECollisionEnabled::NoCollision )
		{
			CollisionReponse = EBodyCollisionResponse::BodyCollision_Disabled;
		}
	}

	// Compress to whatever formats the active target platforms want
	ITargetPlatformManagerModule* TPM = GetTargetPlatformManager();
	if (TPM)
	{
		const TArray<ITargetPlatform*>& Platforms = TPM->GetActiveTargetPlatforms();

		for (int32 Index = 0; Index < Platforms.Num(); Index++)
		{
			GetCookedData(Platforms[Index]->GetPhysicsFormat(this));
		}
	}

	// make sure that we load the physX data while the linker's loader is still open
	CreatePhysicsMeshes();

	// fix up invalid transform to use identity
	// this can be here because BodySetup isn't blueprintable
	if ( GetLinkerUE4Version() < VER_UE4_FIXUP_BODYSETUP_INVALID_CONVEX_TRANSFORM )
	{
		for (int32 i=0; i<AggGeom.ConvexElems.Num(); ++i)
		{
			if ( AggGeom.ConvexElems[i].GetTransform().IsValid() == false )
			{
				AggGeom.ConvexElems[i].SetTransform(FTransform::Identity);
			}
		}
	}
}


FByteBulkData* UBodySetup::GetCookedData(FName Format)
{
	if (IsTemplate())
	{
		return NULL;
	}

	IInterface_CollisionDataProvider* CDP = InterfaceCast<IInterface_CollisionDataProvider>(GetOuter());

	// If there is nothing to cook or if we are reading data from a cooked package for an asset with no collision, 
	// we want to return here
	if ((AggGeom.ConvexElems.Num() == 0 && CDP == NULL) || !bHasCookedCollisionData)
	{
		return NULL;
	}

	bool bContainedData = CookedFormatData.Contains(Format);
	FByteBulkData* Result = &CookedFormatData.GetFormat(Format);
#if WITH_PHYSX
	if (!bContainedData)
	{
		if (FPlatformProperties::RequiresCookedData())
		{
			UE_LOG(LogPhysics, Error, TEXT("Attempt to build physics data for %s when we are unable to. This platform requires cooked packages."), *GetPathName());
		}

		if (AggGeom.ConvexElems.Num() == 0 && (CDP == NULL || CDP->ContainsPhysicsTriMeshData(bMeshCollideAll) == false))
		{
			return NULL;
		}

#if WITH_EDITOR
		TArray<uint8> OutData;
		FDerivedDataPhysXCooker* DerivedPhysXData = new FDerivedDataPhysXCooker(Format, this);
		if (DerivedPhysXData->CanBuild())
		{
			GetDerivedDataCacheRef().GetSynchronous(DerivedPhysXData, OutData);
			if (OutData.Num())
			{
				Result->Lock(LOCK_READ_WRITE);
				FMemory::Memcpy(Result->Realloc(OutData.Num()), OutData.GetTypedData(), OutData.Num());
				Result->Unlock();
			}
		}
		else
#endif
		{
			UE_LOG(LogPhysics, Warning, TEXT("Attempt to build physics data for %s when we are unable to."), *GetPathName());
		}
	}
#endif // WITH_PHYSX
	check(Result);
	return Result->GetBulkDataSize() > 0 ? Result : NULL; // we don't return empty bulk data...but we save it to avoid thrashing the DDC
}

void UBodySetup::PostInitProperties()
{
	Super::PostInitProperties();

	if(!IsTemplate())
	{
		BodySetupGuid = FGuid::NewGuid();
	}
}

SIZE_T UBodySetup::GetResourceSize( EResourceSizeMode::Type Mode )
{
	SIZE_T ResourceSize = 0;

#if WITH_PHYSX
	// Count PhysX trimesh mem usage
	if (TriMesh != NULL)
	{
		ResourceSize += GetPhysxObjectSize(TriMesh, NULL);
	}

	if (TriMeshNegX != NULL)
	{
		ResourceSize += GetPhysxObjectSize(TriMeshNegX, NULL);
	}

	// Count PhysX convex mem usage
	for(int ConvIdx=0; ConvIdx<AggGeom.ConvexElems.Num(); ConvIdx++)
	{
		FKConvexElem& ConvexElem = AggGeom.ConvexElems[ConvIdx];

		if(ConvexElem.ConvexMesh != NULL)
		{
			ResourceSize += GetPhysxObjectSize(ConvexElem.ConvexMesh, NULL);
		}

		if(ConvexElem.ConvexMeshNegX != NULL)
		{
			ResourceSize += GetPhysxObjectSize(ConvexElem.ConvexMeshNegX, NULL);
		}
	}

#endif // WITH_PHYSX

	if (CookedFormatData.Contains(FPlatformProperties::GetPhysicsFormat()))
	{
		const FByteBulkData& FmtData = CookedFormatData.GetFormat(FPlatformProperties::GetPhysicsFormat());
		ResourceSize += FmtData.GetElementSize() * FmtData.GetElementCount();
	}
	
	return ResourceSize;
}

void FKAggregateGeom::Serialize( const FArchive& Ar )
{
	if ( Ar.IsLoading() && Ar.UE4Ver() < VER_UE4_REFACTOR_PHYSICS_TRANSFORMS )
	{
		for ( auto SphereElemIt = SphereElems.CreateIterator(); SphereElemIt; ++SphereElemIt )
		{
			SphereElemIt->Serialize( Ar );
		}

		for ( auto BoxElemIt = BoxElems.CreateIterator(); BoxElemIt; ++BoxElemIt )
		{
			BoxElemIt->Serialize( Ar );
		}

		for ( auto SphylElemIt = SphylElems.CreateIterator(); SphylElemIt; ++SphylElemIt )
		{
			SphylElemIt->Serialize( Ar );
		}
	}
}

float FKAggregateGeom::GetVolume(const FVector& Scale) const
{
	float Volume = 0.0f;

	for ( auto SphereElemIt = SphereElems.CreateConstIterator(); SphereElemIt; ++SphereElemIt )
	{
		Volume += SphereElemIt->GetVolume(Scale);
	}

	for ( auto BoxElemIt = BoxElems.CreateConstIterator(); BoxElemIt; ++BoxElemIt )
	{
		Volume += BoxElemIt->GetVolume(Scale);
	}

	for ( auto SphylElemIt = SphylElems.CreateConstIterator(); SphylElemIt; ++SphylElemIt )
	{
		Volume += SphylElemIt->GetVolume(Scale);
	}

	for ( auto ConvexElemIt = ConvexElems.CreateConstIterator(); ConvexElemIt; ++ConvexElemIt )
	{
		Volume += ConvexElemIt->GetVolume(Scale);
	}

	return Volume;
}

int32 FKAggregateGeom::GetElementCount(int32 Type) const
{
	switch (Type)
	{
	case KPT_Box:
		return BoxElems.Num();
	case KPT_Convex:
		return ConvexElems.Num();
	case KPT_Sphyl:
		return SphylElems.Num();
	case KPT_Sphere:
		return SphereElems.Num();
	default:
		return 0;
	}
}

// References: 
// http://amp.ece.cmu.edu/Publication/Cha/icip01_Cha.pdf
// http://stackoverflow.com/questions/1406029/how-to-calculate-the-volume-of-a-3d-mesh-object-the-surface-of-which-is-made-up
float SignedVolumeOfTriangle(const FVector& p1, const FVector& p2, const FVector& p3) 
{
	return FVector::DotProduct(p1, FVector::CrossProduct(p2, p3)) / 6.0f;
}

float FKConvexElem::GetVolume(const FVector& Scale) const
{
	float Volume = 0.0f;

#if WITH_PHYSX
	if (ConvexMesh != NULL)
	{
		// Preparation for convex mesh scaling implemented in another changelist
		FTransform ScaleTransform = FTransform(FQuat::Identity, FVector::ZeroVector, Scale);

		int32 NumPolys = ConvexMesh->getNbPolygons();
		PxHullPolygon PolyData;

		const PxVec3* Vertices = ConvexMesh->getVertices();
		const PxU8* Indices = ConvexMesh->getIndexBuffer();

		for (int32 PolyIdx = 0; PolyIdx < NumPolys; ++PolyIdx)
		{
			if (ConvexMesh->getPolygonData(PolyIdx, PolyData))
			{
				for (int32 VertIdx = 2; VertIdx < PolyData.mNbVerts; ++ VertIdx)
				{
					// Grab triangle indices that we hit
					int32 I0 = Indices[PolyData.mIndexBase + 0];
					int32 I1 = Indices[PolyData.mIndexBase + (VertIdx - 1)];
					int32 I2 = Indices[PolyData.mIndexBase + VertIdx];


					Volume += SignedVolumeOfTriangle(ScaleTransform.TransformPosition(P2UVector(Vertices[I0])), 
						ScaleTransform.TransformPosition(P2UVector(Vertices[I1])), 
						ScaleTransform.TransformPosition(P2UVector(Vertices[I2])));
				}
			}
		}
	}
#endif // WITH_PHYSX

	return Volume;
}

void FKSphereElem::Serialize( const FArchive& Ar )
{
	if ( Ar.IsLoading() && Ar.UE4Ver() < VER_UE4_REFACTOR_PHYSICS_TRANSFORMS )
	{
		Center = TM_DEPRECATED.GetOrigin();
	}
}

void FKBoxElem::Serialize( const FArchive& Ar )
{
	if ( Ar.IsLoading() && Ar.UE4Ver() < VER_UE4_REFACTOR_PHYSICS_TRANSFORMS )
	{
		Center = TM_DEPRECATED.GetOrigin();
		Orientation = TM_DEPRECATED.ToQuat();
	}
}

void FKSphylElem::Serialize( const FArchive& Ar )
{
	if ( Ar.IsLoading() && Ar.UE4Ver() < VER_UE4_REFACTOR_PHYSICS_TRANSFORMS )
	{
		Center = TM_DEPRECATED.GetOrigin();
		Orientation = TM_DEPRECATED.ToQuat();
	}
}

class UPhysicalMaterial* UBodySetup::GetPhysMaterial() const
{
	UPhysicalMaterial* PhysMat = PhysMaterial;

	if (PhysMat == NULL && GEngine != NULL)
	{
		PhysMat = GEngine->DefaultPhysMaterial;
	}
	return PhysMat;
}

float UBodySetup::CalculateMass(const UPrimitiveComponent* Component)
{
	FVector ComponentScale(1.0f, 1.0f, 1.0f);
	UPhysicalMaterial* PhysMat = GetPhysMaterial();
	const FBodyInstance* BodyInstance = NULL;
	float MassScale = DefaultInstance.MassScale;

	const UPrimitiveComponent* OuterComp = Component != NULL ? Component : Cast<UPrimitiveComponent>(GetOuter());
	if (OuterComp)
	{
		ComponentScale = OuterComp->GetComponentScale();

		BodyInstance = &OuterComp->BodyInstance;

		const USkinnedMeshComponent* SkinnedMeshComp = Cast<const USkinnedMeshComponent>(OuterComp);
		if (SkinnedMeshComp != NULL)
		{
			const FBodyInstance* Body = SkinnedMeshComp->GetBodyInstance(BoneName);

			if (Body != NULL)
			{
				BodyInstance = Body;
			}
		}
	}
	else
	{
		BodyInstance = &DefaultInstance;
	}

	PhysMat = BodyInstance->PhysMaterialOverride != NULL ? BodyInstance->PhysMaterialOverride : PhysMat;
	MassScale = BodyInstance->MassScale;

	// physical material - nothing can weigh less than hydrogen (0.09 kg/m^3)
	float DensityKGPerCubicUU = 1.0f;
	float RaiseMassToPower = 0.75f;
	if (PhysMat)
	{
		DensityKGPerCubicUU = FMath::Max(0.00009f, PhysMat->Density * 0.001f);
		RaiseMassToPower = PhysMat->RaiseMassToPower;
	}

	// Then scale mass to avoid big differences between big and small objects.
	float BasicMass = AggGeom.GetVolume(ComponentScale) * DensityKGPerCubicUU;

	float UsePow = FMath::Clamp<float>(RaiseMassToPower, KINDA_SMALL_NUMBER, 1.f);
	float RealMass = FMath::Pow(BasicMass, UsePow);

	return RealMass * MassScale;
}
