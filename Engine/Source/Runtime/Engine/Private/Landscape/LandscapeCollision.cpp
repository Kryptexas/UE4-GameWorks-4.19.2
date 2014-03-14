// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
LandscapeCollision.cpp: Landscape collision
=============================================================================*/

#include "EnginePrivate.h"
#include "Landscape/LandscapeDataAccess.h"
#include "Landscape/LandscapeRender.h"
#include "../PhysicsEngine/PhysXSupport.h"
#include "../Collision/PhysXCollision.h"
#include "DerivedDataPluginInterface.h"
#include "DerivedDataCacheInterface.h"
#include "../PhysicsEngine/PhysDerivedData.h"

TMap<FGuid, ULandscapeHeightfieldCollisionComponent::FPhysXHeightfieldRef* > GSharedHeightfieldRefs;

ULandscapeHeightfieldCollisionComponent::FPhysXHeightfieldRef::~FPhysXHeightfieldRef()
{
#if WITH_PHYSX
	// Free the existing heightfield data.
	if( RBHeightfield )
	{
		GPhysXPendingKillHeightfield.Add(RBHeightfield);
		RBHeightfield = NULL;
	}
#endif
	// Remove ourselves from the shared map.
	GSharedHeightfieldRefs.Remove(Guid);
}

TMap<FGuid, ULandscapeMeshCollisionComponent::FPhysXMeshRef* > GSharedMeshRefs;

ULandscapeMeshCollisionComponent::FPhysXMeshRef::~FPhysXMeshRef()
{
#if WITH_PHYSX
	// Free the existing heightfield data.
	if( RBTriangleMesh )
	{
		GPhysXPendingKillTriMesh.Add(RBTriangleMesh);
		RBTriangleMesh = NULL;
	}
#endif
	// Remove ourselves from the shared map.
	GSharedMeshRefs.Remove(Guid);
}

ECollisionEnabled::Type ULandscapeHeightfieldCollisionComponent::GetCollisionEnabled() const
{
	ALandscapeProxy* Proxy = GetLandscapeProxy();

	return Proxy->BodyInstance.GetCollisionEnabled();
}

ECollisionResponse ULandscapeHeightfieldCollisionComponent::GetCollisionResponseToChannel(ECollisionChannel Channel) const
{
	ALandscapeProxy* Proxy = GetLandscapeProxy();

	return Proxy->BodyInstance.GetResponseToChannel(Channel);
}

ECollisionChannel ULandscapeHeightfieldCollisionComponent::GetCollisionObjectType() const
{
	ALandscapeProxy* Proxy = GetLandscapeProxy();

	return Proxy->BodyInstance.GetObjectType();
}

const FCollisionResponseContainer& ULandscapeHeightfieldCollisionComponent::GetCollisionResponseToChannels() const
{
	ALandscapeProxy* Proxy = GetLandscapeProxy();

	return Proxy->BodyInstance.GetResponseToChannels();
}

void ULandscapeHeightfieldCollisionComponent::CreatePhysicsState()
{
	USceneComponent::CreatePhysicsState(); // route CreatePhysicsState, skip PrimitiveComponent implementation

	if( !BodyInstance.IsValidBodyInstance())
	{
#if WITH_PHYSX
		int32 CollisionSizeVerts = CollisionSizeQuads+1;

		// Make transform for this landscape component PxActor
		FTransform LandscapeComponentTransform = GetComponentToWorld();
		FMatrix LandscapeComponentMatrix = LandscapeComponentTransform.ToMatrixWithScale();

		bool bIsMirrored = LandscapeComponentMatrix.Determinant() < 0.f;

		if( !bIsMirrored )
		{
			// Unreal and PhysX have opposite handedness, so we need to translate the origin and rearrange the data
			LandscapeComponentMatrix = FTranslationMatrix(FVector(CollisionSizeQuads*CollisionScale,0,0)) * LandscapeComponentMatrix;
		}		

		// Get the scale to give to PhysX
		FVector LandscapeScale = LandscapeComponentMatrix.ExtractScaling();

		// Reorder the axes
		FVector TerrainX = LandscapeComponentMatrix.GetScaledAxis( EAxis::X );
		FVector TerrainY = LandscapeComponentMatrix.GetScaledAxis( EAxis::Y );
		FVector TerrainZ = LandscapeComponentMatrix.GetScaledAxis( EAxis::Z );
		LandscapeComponentMatrix.SetAxis(0, TerrainX);
		LandscapeComponentMatrix.SetAxis(2, TerrainY);
		LandscapeComponentMatrix.SetAxis(1, TerrainZ);
		
		PxTransform PhysXLandscapeComponentTransform = U2PTransform(FTransform(LandscapeComponentMatrix));

		check(GEngine->DefaultPhysMaterial);

		// If we have not created a heightfield yet - do it now.
		if( !IsValidRef(HeightfieldRef) )
		{
			if( !HeightfieldGuid.IsValid() )
			{
				HeightfieldGuid = FGuid::NewGuid();
			}

			// Look for a heightfield object with the current Guid (this occurs with PIE)
			FPhysXHeightfieldRef* ExistingHeightfieldRef = GSharedHeightfieldRefs.FindRef(HeightfieldGuid);
			if( ExistingHeightfieldRef )
			{
				HeightfieldRef = ExistingHeightfieldRef;
			}
			else
			{
				HeightfieldRef = GSharedHeightfieldRefs.Add( HeightfieldGuid, new FPhysXHeightfieldRef(HeightfieldGuid) );

				ALandscapeProxy* Proxy = GetLandscapeProxy();

				// Default physical material used for regions where the dominant layer has no PhysicalMaterial set.
				PxMaterial* DefPxMaterial = ((Proxy && Proxy->DefaultPhysMaterial!=NULL) ? Proxy->DefaultPhysMaterial : GEngine->DefaultPhysMaterial)->GetPhysXMaterial();

				TArray<PxMaterial*> LayerPxMaterials;
				LayerPxMaterials.AddZeroed(ComponentLayerInfos.Num());
				int32 HoleLayerIdx = INDEX_NONE;

				// Necessary for background navimesh building
				FScopeLock ScopeLock(&CollisionDataSyncObject);

				// Look up the physical materials we'll need
				HeightfieldRef->UsedPhysicalMaterialArray.Empty();
				uint8* DominantLayers = NULL;
				if( Proxy != NULL && DominantLayerData.GetElementCount() > 0 )
				{
					for( int32 Idx=0;Idx<ComponentLayerInfos.Num();Idx++ )
					{
						if (ComponentLayerInfos[Idx] == ALandscapeProxy::DataLayer)
						{
							bIncludeHoles = true;
							if(!GIsEditor || bHeightFieldDataHasHole)
							{
								HoleLayerIdx = Idx;
							}
						}
						else
						{
							ULandscapeLayerInfoObject* LayerInfo = ComponentLayerInfos[Idx];

							if( LayerInfo && LayerInfo->PhysMaterial )
							{
								LayerPxMaterials[Idx] = LayerInfo->PhysMaterial->GetPhysXMaterial();
							}
						}
					}

					DominantLayers = (uint8*)DominantLayerData.Lock(LOCK_READ_ONLY);
				}
				else
				{
					// Add the default physical material to be used used when we have no dominant data.
					HeightfieldRef->UsedPhysicalMaterialArray.Add(DefPxMaterial);
				}


				uint16* Heights = (uint16*)CollisionHeightData.Lock(LOCK_READ_ONLY);
				check(CollisionHeightData.GetElementCount()==FMath::Square(CollisionSizeVerts));

				TArray<PxHeightFieldSample> Samples;
				Samples.AddZeroed(FMath::Square(CollisionSizeVerts));

				for(int32 RowIndex = 0; RowIndex < CollisionSizeVerts; RowIndex++)
				{
					for(int32 ColIndex = 0; ColIndex < CollisionSizeVerts; ColIndex++)
					{
						int32 SrcSampleIndex = (ColIndex * CollisionSizeVerts) + (bIsMirrored ? RowIndex : (CollisionSizeVerts-RowIndex-1));
						int32 DstSampleIndex = (RowIndex * CollisionSizeVerts) + ColIndex;

						PxHeightFieldSample& Sample = Samples[DstSampleIndex];
						Sample.height = FMath::Clamp<int16>(((int32)Heights[SrcSampleIndex]-32768), -32678, 32767);

						// Materials are not relevant on the last row/column because they are per-triangle and the last row/column don't own any
						if (RowIndex < CollisionSizeVerts - 1 &&
							ColIndex < CollisionSizeVerts - 1)
						{
							int32 MaterialIndex;
							if( DominantLayers )
							{
								uint8 DominantLayerIdx = DominantLayers[SrcSampleIndex];

								// If it's a hole, override with the hole flag.
								if( DominantLayerIdx == HoleLayerIdx )
								{
									MaterialIndex = PxHeightFieldMaterial::eHOLE;
								}
								else
								{
									PxMaterial* DominantPxMaterial = NULL;
									if( DominantLayerIdx < LayerPxMaterials.Num() )
									{
										DominantPxMaterial = LayerPxMaterials[DominantLayerIdx];
									}

									if( DominantPxMaterial == NULL )
									{
										DominantPxMaterial = DefPxMaterial;
									}

									MaterialIndex = HeightfieldRef->UsedPhysicalMaterialArray.Find(DominantPxMaterial);
									if( MaterialIndex == INDEX_NONE )
									{
										MaterialIndex = HeightfieldRef->UsedPhysicalMaterialArray.Add(DominantPxMaterial);
									}
								}
							}
							else
							{
								// Use the default physical material.
								MaterialIndex = 0;
							}

							Sample.materialIndex0 = MaterialIndex;
							Sample.materialIndex1 = MaterialIndex;
						}

						// TODO: edge turning
					}
				}

				PxHeightFieldDesc HFDesc;
				HFDesc.format           = PxHeightFieldFormat::eS16_TM;
				HFDesc.nbColumns		= CollisionSizeVerts;
				HFDesc.nbRows			= CollisionSizeVerts;
				HFDesc.samples.data		= Samples.GetData();
				HFDesc.samples.stride	= sizeof(PxHeightFieldSample);
				HFDesc.flags			= PxHeightFieldFlag::eNO_BOUNDARY_EDGES;
				HFDesc.thickness        = -Proxy->CollisionThickness / (LandscapeScale.Z * LANDSCAPE_ZSCALE);

				HeightfieldRef->RBHeightfield = GPhysXSDK->createHeightField(HFDesc);

				CollisionHeightData.Unlock();
				if( DominantLayers )
				{
					DominantLayerData.Unlock();
				}

				// When a component is totally hole
				// TODO: Need to remove component 
				if (HeightfieldRef->UsedPhysicalMaterialArray.Num() == 0) 
				{
					HeightfieldRef->UsedPhysicalMaterialArray.Add(DefPxMaterial);
				}
			}
		}

		check( IsValidRef(HeightfieldRef) );

		// Create the geometry
		PxHeightFieldGeometry LandscapeComponentGeom( HeightfieldRef->RBHeightfield, PxMeshGeometryFlags(), LandscapeScale.Z * LANDSCAPE_ZSCALE, LandscapeScale.Y * CollisionScale, LandscapeScale.X * CollisionScale );
		check(LandscapeComponentGeom.isValid());

		// Creating both a sync and async actor, since this object is static

		// Create the sync scene actor
		PxRigidStatic* HeightFieldActorSync = GPhysXSDK->createRigidStatic(PhysXLandscapeComponentTransform);
		PxShape* HeightFieldShapeSync = HeightFieldActorSync->createShape(LandscapeComponentGeom, HeightfieldRef->UsedPhysicalMaterialArray.GetTypedData(), HeightfieldRef->UsedPhysicalMaterialArray.Num());
		check(HeightFieldShapeSync);

		// Setup filtering
		PxFilterData PQueryFilterData, PSimFilterData;
		CreateShapeFilterData( GetCollisionObjectType(), GetUniqueID(), GetCollisionResponseToChannels(), 0, 0, PQueryFilterData, PSimFilterData, true, false, true);

		// Heightfield is used for simple and complex collision
		PQueryFilterData.word3 |= EPDF_SimpleCollision | EPDF_ComplexCollision;
		HeightFieldShapeSync->setQueryFilterData(PQueryFilterData);
		HeightFieldShapeSync->setSimulationFilterData(PSimFilterData);
		HeightFieldShapeSync->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, true); 
		HeightFieldShapeSync->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true); 
		HeightFieldShapeSync->setFlag(PxShapeFlag::eVISUALIZATION, true);

		// Create the async scene actor
		PxRigidStatic* HeightFieldActorAsync = GPhysXSDK->createRigidStatic(PhysXLandscapeComponentTransform);
		PxShape* HeightFieldShapeAsync = HeightFieldActorAsync->createShape(LandscapeComponentGeom, HeightfieldRef->UsedPhysicalMaterialArray.GetTypedData(), HeightfieldRef->UsedPhysicalMaterialArray.Num());
		HeightFieldShapeAsync->setContactOffset(GPhysXCooking->getParams().skinWidth);
		
		HeightFieldShapeAsync->setQueryFilterData(PQueryFilterData);
		HeightFieldShapeAsync->setSimulationFilterData(PSimFilterData);
		// Only perform scene queries in the synchronous scene for static shapes
		HeightFieldShapeAsync->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, false);
		HeightFieldShapeAsync->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true);	
		HeightFieldShapeAsync->setFlag(PxShapeFlag::eVISUALIZATION, true);

		FPhysScene* PhysScene = GetWorld()->GetPhysicsScene();

		// Set body instance data
		BodyInstance.OwnerComponent = this;
		BodyInstance.SceneIndexSync = PhysScene->PhysXSceneIndex[PST_Sync];
		BodyInstance.SceneIndexAsync = PhysScene->PhysXSceneIndex[PST_Async];
		BodyInstance.RigidActorSync = HeightFieldActorSync;
		BodyInstance.RigidActorAsync = HeightFieldActorAsync;
		HeightFieldActorSync->userData = &BodyInstance.PhysxUserData;
		HeightFieldActorAsync->userData = &BodyInstance.PhysxUserData;

		// Add to scenes
		PhysScene->GetPhysXScene(PST_Sync)->addActor(*HeightFieldActorSync);

		if (PhysScene->HasAsyncScene())
		{
			PxScene* AsyncScene = PhysScene->GetPhysXScene(PST_Async);
			
			SCOPED_SCENE_WRITE_LOCK( AsyncScene );
			AsyncScene->addActor(*HeightFieldActorAsync);
		}
#endif
	}
}

void ULandscapeHeightfieldCollisionComponent::ApplyWorldOffset(const FVector& InOffset, bool bWorldShift)
{
	Super::ApplyWorldOffset(InOffset, bWorldShift);

	if (!bWorldShift || !FPhysScene::SupportsOriginShifting())
	{
		RecreateCollision(false);
	}
}

TArray<int32> ULandscapeMeshCollisionComponent::UpdateMaterials()
{
	TArray<int32> LayerPhysMaterialIndices;
#if WITH_PHYSX
	if( !IsValidRef(MeshRef))
	{
		return LayerPhysMaterialIndices;
	}

	MeshRef->UsedPhysicalMaterialArray.Empty();
			
	// Phys Material setup
	ALandscapeProxy* Proxy = GetLandscapeProxy();

	// Default physical material used for regions where the dominant layer has no PhysicalMaterial set.
	PxMaterial* DefPxMaterial = ((Proxy && Proxy->DefaultPhysMaterial!=NULL) ? Proxy->DefaultPhysMaterial : GEngine->DefaultPhysMaterial)->GetPhysXMaterial();

	// Always add the default physical material to be used used for regions where the dominant layer has no PhysicalMaterial set.
	MeshRef->UsedPhysicalMaterialArray.Add(DefPxMaterial);

	LayerPhysMaterialIndices.AddZeroed(ComponentLayerInfos.Num());

	if( Proxy != NULL )
	{
		for( int32 Idx=0;Idx<ComponentLayerInfos.Num();Idx++ )
		{
			ULandscapeLayerInfoObject* LayerInfo = ComponentLayerInfos[Idx];

			if (LayerInfo == ALandscapeProxy::DataLayer)
			{
				bIncludeHoles = true;
				LayerPhysMaterialIndices[Idx] = 255;
			}
			else if( LayerInfo && LayerInfo->PhysMaterial )
			{
				PxMaterial* LayerPxMaterial = LayerInfo->PhysMaterial->GetPhysXMaterial();
				int32 PhysMatIdx = MeshRef->UsedPhysicalMaterialArray.Find(LayerPxMaterial);
				if( PhysMatIdx == INDEX_NONE )
				{
					PhysMatIdx = MeshRef->UsedPhysicalMaterialArray.Add(LayerPxMaterial);
				}
				LayerPhysMaterialIndices[Idx] = PhysMatIdx;
			}
			else
			{
				LayerPhysMaterialIndices[Idx] = 0;				
			}
		}
	}
#endif
	return LayerPhysMaterialIndices;
}

bool ULandscapeMeshCollisionComponent::GetPhysicsTriMeshData(struct FTriMeshCollisionData* CollisionData, bool InUseAllTriData)
{
	const int32 CollisionSizeVerts = CollisionSizeQuads+1;
	const uint32 NumVerts = FMath::Square(CollisionSizeVerts);

	// Necessary for background navimesh building
	FScopeLock ScopeLock(&CollisionDataSyncObject);
	uint16* Heights = (uint16*)CollisionHeightData.Lock(LOCK_READ_ONLY);
	uint16* XYOffsets = (uint16*)CollisionXYOffsetData.Lock(LOCK_READ_ONLY);
	check(CollisionHeightData.GetElementCount()==NumVerts);
	check(CollisionXYOffsetData.GetElementCount()==NumVerts*2);

	check( IsValidRef(MeshRef) );
	
	TArray<int32> LayerPhysMaterialIndices = UpdateMaterials();
	uint8* DominantLayers = DominantLayerData.GetElementCount() > 0 ? (uint8*)DominantLayerData.Lock(LOCK_READ_ONLY) : NULL;

	// Scale all verts into temporary vertex buffer.
	CollisionData->Vertices.Empty();
	CollisionData->Vertices.AddUninitialized(NumVerts);
	for(uint32 i=0; i<NumVerts; i++)
	{
		int32 X = i % CollisionSizeVerts;
		int32 Y = i / CollisionSizeVerts;
		CollisionData->Vertices[i].Set(X + ((float)XYOffsets[i*2] - 32768.f) * LANDSCAPE_XYOFFSET_SCALE, Y + ((float)XYOffsets[i*2+1] - 32768.f) * LANDSCAPE_XYOFFSET_SCALE, ((float) Heights[i] - 32768.f) * LANDSCAPE_ZSCALE );
	}

	const uint32 NumTris = FMath::Square(CollisionSizeQuads) * 2;
	CollisionData->Indices.Empty();
	CollisionData->Indices.AddUninitialized(NumTris);
	if (DominantLayers)
	{
		CollisionData->MaterialIndices.Empty();
		CollisionData->MaterialIndices.AddUninitialized(NumTris);
	}

	{
		uint32 i = 0;
		for( int32 y=0;y<CollisionSizeQuads;y++ )
		{
			for( int32 x=0;x<CollisionSizeQuads;x++ )
			{
				int32 DataIdx = x + y * CollisionSizeVerts;
				int32 MaterialIndex = (DominantLayers && DominantLayers[DataIdx] != 255 && LayerPhysMaterialIndices.Num()) ? LayerPhysMaterialIndices[DominantLayers[DataIdx]] : 0;
				bool bHasHole = bHeightFieldDataHasHole && (MaterialIndex == 255);

				FTriIndices& TriIndex1 = CollisionData->Indices[ i ];
				if (bHasHole)
				{
					TriIndex1.v0 = 0;
					TriIndex1.v1 = 0;
					TriIndex1.v2 = 0;
				}
				else
				{
				TriIndex1.v0 = (x+0) + (y+0) * CollisionSizeVerts;
				TriIndex1.v1 = (x+1) + (y+1) * CollisionSizeVerts;
				TriIndex1.v2 = (x+1) + (y+0) * CollisionSizeVerts;
				}

				if (DominantLayers)
				{
					CollisionData->MaterialIndices[ i ] = MaterialIndex;
				}
				i++;

				FTriIndices& TriIndex2 = CollisionData->Indices[ i ];
				if (bHasHole)
				{
					TriIndex2.v0 = 0;
					TriIndex2.v1 = 0;
					TriIndex2.v2 = 0;
				}
				else
				{
				TriIndex2.v0 = (x+0) + (y+0) * CollisionSizeVerts;
				TriIndex2.v1 = (x+0) + (y+1) * CollisionSizeVerts;
				TriIndex2.v2 = (x+1) + (y+1) * CollisionSizeVerts;
				}

				if (DominantLayers)
				{
					CollisionData->MaterialIndices[ i ] = MaterialIndex;
				}
				i++;
			}
		}
	}

	CollisionData->bFlipNormals = true;

	CollisionHeightData.Unlock();
	CollisionXYOffsetData.Unlock();
	if( DominantLayers )
	{
		DominantLayerData.Unlock();
	}

	return true;
}

bool ULandscapeMeshCollisionComponent::ContainsPhysicsTriMeshData(bool InUseAllTriData) const 
{ 
	return CollisionSizeQuads > 0; 
}

FByteBulkData* ULandscapeMeshCollisionComponent::GetCookedData(FName Format, bool bIsMirrored)
{
	if (IsTemplate())
	{
		return NULL;
	}

	bool bContainedData = CookedFormatData.Contains(Format);
	FByteBulkData& Result = CookedFormatData.GetFormat(Format);
#if WITH_PHYSX
	// If platform supports cooking PhysX data, always regenerate the cooked data to ensure we have the latest 
	// (eg in case of editor)
	if (!bContainedData || !FPlatformProperties::RequiresCookedData())
	{
		if (FPlatformProperties::RequiresCookedData())
		{
			UE_LOG(LogPhysics, Fatal, TEXT("Attempt to build physics data for %s when we are unable to. This platform requires cooked packages."), *GetPathName());
		}

#if WITH_EDITOR
		FDerivedDataPhysXCooker DerivedTriMeshData(Format, this, bIsMirrored);
		if (DerivedTriMeshData.CanBuild())
		{
			TArray<uint8> OutData;
			// Don't use DDC to reduce file number generated by frequent changes during editing (only for editor case)
			DerivedTriMeshData.Build(OutData);

			if (OutData.Num())
			{
				Result.Lock(LOCK_READ_WRITE);
				FMemory::Memcpy(Result.Realloc(OutData.Num()), OutData.GetTypedData(), OutData.Num());
				Result.Unlock();
			}
		}
		else
#endif
		{
			UE_LOG(LogPhysics, Warning, TEXT("Attempt to cooking physX data when no cooker is available %s."), *GetPathName());
		}
	}
#endif // WITH_PHYSX
	return Result.GetBulkDataSize() > 0 ? &Result : NULL; // we don't return empty bulk data...but we save it to avoid thrashing the DDC
}

void ULandscapeMeshCollisionComponent::CreatePhysicsState()
{
	// Just call ULandscapeHeightfieldCollisionComponent::CreatePhysicsState() here to use projected height field...
	USceneComponent::CreatePhysicsState(); // route CreatePhysicsState, skip PrimitiveComponent implementation

	if(!BodyInstance.IsValidBodyInstance())
	{
#if WITH_PHYSX
		int32 CollisionSizeVerts = CollisionSizeQuads+1;

		// Make transform for this landscape component PxActor
		FTransform LandscapeComponentTransform = GetComponentToWorld();
		FMatrix LandscapeComponentMatrix = LandscapeComponentTransform.ToMatrixWithScale();

		bool bIsMirrored = LandscapeComponentMatrix.Determinant() < 0.f;

		if( bIsMirrored )
		{
			// Unreal and PhysX have opposite handedness, so we need to translate the origin and rearrange the data
			LandscapeComponentMatrix = FTranslationMatrix(FVector(CollisionSizeVerts-1,0,0)) * LandscapeComponentMatrix;
		}		

		// Get the scale to give to PhysX
		FVector LandscapeScale = LandscapeComponentMatrix.ExtractScaling();

		PxTransform PhysXLandscapeComponentTransform = U2PTransform(FTransform(LandscapeComponentMatrix));

		check(GEngine->DefaultPhysMaterial);

		// If we have not created a heightfield yet - do it now.
		if( !IsValidRef(MeshRef) )
		{
			if( !MeshGuid.IsValid() )
			{
				MeshGuid = FGuid::NewGuid();
			}

			// Look for a heightfield object with the current Guid (this occurs with PIE)
			FPhysXMeshRef* ExistingMeshRef = GSharedMeshRefs.FindRef(MeshGuid);
			if( ExistingMeshRef )
			{
				MeshRef = ExistingMeshRef;
			}
			else			
			{
				MeshRef = GSharedMeshRefs.Add( MeshGuid, new FPhysXMeshRef(MeshGuid) );

				// Find or create cooked physics data
				static FName PhysicsFormatName(FPlatformProperties::GetPhysicsFormat());
				FByteBulkData* FormatData = GetCookedData(PhysicsFormatName, bIsMirrored);

				if (FormatData)
				{
					if (FormatData->IsLocked())
					{
						// seems it's being already processed
						return;
					}
					// Create physics objects
					FPhysXFormatDataReader CookedDataReader( *FormatData );
					MeshRef->RBTriangleMesh = CookedDataReader.TriMesh;
				}
			}
		}

		check( IsValidRef(MeshRef) );

		// Create tri-mesh shape
		if(MeshRef->RBTriangleMesh != NULL)
		{
			PxTriangleMeshGeometry PTriMeshGeom;
			PTriMeshGeom.triangleMesh = MeshRef->RBTriangleMesh;
			PTriMeshGeom.scale.scale.x = LandscapeScale.X * CollisionScale;
			PTriMeshGeom.scale.scale.y = LandscapeScale.Y * CollisionScale;
			PTriMeshGeom.scale.scale.z = LandscapeScale.Z;

			if(PTriMeshGeom.isValid())
			{
				// Creating both a sync and async actor, since this object is static
				UpdateMaterials();

				// Create the sync scene actor
				PxRigidStatic* MeshActorSync = GPhysXSDK->createRigidStatic(PhysXLandscapeComponentTransform);
				PxShape* MeshShapeSync = MeshActorSync->createShape(PTriMeshGeom, MeshRef->UsedPhysicalMaterialArray.GetTypedData(), MeshRef->UsedPhysicalMaterialArray.Num());
				MeshShapeSync->setContactOffset(GPhysXCooking->getParams().skinWidth);
				check(MeshShapeSync);

				// Setup filtering
				PxFilterData PQueryFilterData, PSimFilterData;
				CreateShapeFilterData( GetCollisionObjectType(), GetOwner()->GetUniqueID(), GetCollisionResponseToChannels(), 0, 0, PQueryFilterData, PSimFilterData, false, false, true);

				// Heightfield is used for simple and complex collision
				PQueryFilterData.word3 |= EPDF_SimpleCollision | EPDF_ComplexCollision;
				MeshShapeSync->setQueryFilterData(PQueryFilterData);

				MeshShapeSync->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true); 
				MeshShapeSync->setFlag(PxShapeFlag::eVISUALIZATION, true);

				// Create the async scene actor
				PxRigidStatic* MeshActorAsync = GPhysXSDK->createRigidStatic(PhysXLandscapeComponentTransform);
				PxShape* MeshShapeAsync = MeshActorAsync->createShape(PTriMeshGeom, MeshRef->UsedPhysicalMaterialArray.GetTypedData(), MeshRef->UsedPhysicalMaterialArray.Num());
				MeshShapeAsync->setContactOffset(GPhysXCooking->getParams().skinWidth);
				check(MeshShapeAsync);

				// No need for query filter data.  That will all be take care of by the sync actor
				MeshShapeAsync->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true); 
				MeshShapeAsync->setFlag(PxShapeFlag::eVISUALIZATION, true);	// Setting visualization flag, in case we visualize only the async scene

				FPhysScene* PhysScene = GetWorld()->GetPhysicsScene();

				// Set body instance data
				BodyInstance.OwnerComponent = this;
				BodyInstance.SceneIndexSync = PhysScene->PhysXSceneIndex[PST_Sync];
				BodyInstance.SceneIndexAsync = PhysScene->PhysXSceneIndex[PST_Async];
				BodyInstance.RigidActorSync = MeshActorSync;
				BodyInstance.RigidActorAsync = MeshActorAsync;
				MeshActorSync->userData = &BodyInstance.PhysxUserData;
				MeshActorAsync->userData = &BodyInstance.PhysxUserData;

				// Add to scenes
				PhysScene->GetPhysXScene(PST_Sync)->addActor(*MeshActorSync);

				if (PhysScene->HasAsyncScene())
				{
					PxScene* AsyncScene = PhysScene->GetPhysXScene(PST_Async);

					SCOPED_SCENE_WRITE_LOCK( AsyncScene );
					AsyncScene->addActor(*MeshActorAsync);
				}
			}
			else
			{
				UE_LOG(LogLandscape, Log, TEXT("ULandscapeMeshCollisionComponent::CreatePhysicsState(): TriMesh invalid"));
			}
		}
#endif // WITH_PHYSX
	}
}

void ULandscapeMeshCollisionComponent::ApplyWorldOffset(const FVector& InOffset, bool bWorldShift)
{
	Super::ApplyWorldOffset(InOffset, bWorldShift);
	
	if (!bWorldShift || !FPhysScene::SupportsOriginShifting())
	{
		RecreateCollision(false);
	}
}

void ULandscapeMeshCollisionComponent::DestroyComponent()
{
	ALandscapeProxy* Proxy = GetLandscapeProxy();
	if (Proxy)
	{
		Proxy->CollisionComponents.Remove(this);
	}

	Super::DestroyComponent();
}

void ULandscapeHeightfieldCollisionComponent::UpdateHeightfieldRegion(int32 ComponentX1, int32 ComponentY1, int32 ComponentX2, int32 ComponentY2)
{
#if WITH_PHYSX
	if( IsValidRef(HeightfieldRef) )
	{
		// If we're currently sharing this data with a PIE session, we need to make a new heightfield.
		if( HeightfieldRef->GetRefCount() > 1 )
		{
			RecreateCollision(false);
			return;
		}

		if (BodyInstance.RigidActorSync == NULL)
		{
			return;
		}

		int32 CollisionSizeVerts = CollisionSizeQuads+1;

		bool bIsMirrored = GetComponentToWorld().GetDeterminant() < 0.f;

		// Necessary for background navimesh building
		FScopeLock ScopeLock(&CollisionDataSyncObject);
		uint16* Heights = (uint16*)CollisionHeightData.Lock(LOCK_READ_ONLY);
		check(CollisionHeightData.GetElementCount()==FMath::Square(CollisionSizeVerts));

		// PhysX heightfield has the X and Y axis swapped, and the X component is also inverted
		int32 HeightfieldX1 = ComponentY1;
		int32 HeightfieldY1 = (bIsMirrored ? ComponentX1 : (CollisionSizeVerts-ComponentX2-1));
		int32 DstVertsX = ComponentY2-ComponentY1+1;
		int32 DstVertsY = ComponentX2-ComponentX1+1;

		TArray<PxHeightFieldSample> Samples;
		Samples.AddZeroed(DstVertsX*DstVertsY);

		// Traverse the area in destination heigthfield coordinates
		for(int32 RowIndex = 0; RowIndex < DstVertsY; RowIndex++)
		{
			for(int32 ColIndex = 0; ColIndex < DstVertsX; ColIndex++)
			{
				int32 SrcX = bIsMirrored ? (RowIndex + ComponentX1) : (ComponentX2 - RowIndex);
				int32 SrcY = ColIndex + ComponentY1;
				int32 SrcSampleIndex = (SrcY * CollisionSizeVerts) + SrcX;
				check( SrcSampleIndex < FMath::Square(CollisionSizeVerts) );
				int32 DstSampleIndex = (RowIndex * DstVertsX) + ColIndex;
				
				PxHeightFieldSample& Sample = Samples[DstSampleIndex];
				Sample.height = FMath::Clamp<int16>(((int32)Heights[SrcSampleIndex]-32768), -32678, 32767);

				// The modifySamples API doesn't yet support updating the material index.
				Sample.materialIndex0 = 0;
				Sample.materialIndex1 = 0;
			}
		}

		PxHeightFieldDesc SubDesc;
		SubDesc.format           = PxHeightFieldFormat::eS16_TM;
		SubDesc.nbColumns		= DstVertsX;
		SubDesc.nbRows			= DstVertsY;
		SubDesc.samples.data	= Samples.GetData();
		SubDesc.samples.stride	= sizeof(PxU32);
		SubDesc.flags			= PxHeightFieldFlag::eNO_BOUNDARY_EDGES;

		FPhysScene* PhysScene = GetWorld()->GetPhysicsScene();

		PxScene* SyncScene = PhysScene->GetPhysXScene(PST_Sync);
		PxScene* AsyncScene = PhysScene->HasAsyncScene() ? PhysScene->GetPhysXScene(PST_Async) : NULL;

		// A remove and re-add to the sync and async scenes seems to be required for the modifySamples
		// bounding box changes to affect the PhysX farfield.
		
		SyncScene->removeActor(*BodyInstance.RigidActorSync);

		if (AsyncScene)
		{
			AsyncScene->lockWrite();

			AsyncScene->removeActor(*BodyInstance.RigidActorAsync);
		}

		HeightfieldRef->RBHeightfield->modifySamples(HeightfieldX1, HeightfieldY1, SubDesc);

		BodyInstance.SceneIndexSync = PhysScene->PhysXSceneIndex[PST_Sync];
		BodyInstance.SceneIndexAsync = PhysScene->PhysXSceneIndex[PST_Async];

		SyncScene->addActor(*BodyInstance.RigidActorSync);

		if (AsyncScene)
		{
			AsyncScene->addActor(*BodyInstance.RigidActorAsync);
			AsyncScene->unlockWrite();
		}

		CollisionHeightData.Unlock();
	}

#endif
}

void ULandscapeHeightfieldCollisionComponent::DestroyComponent()
{
	ALandscapeProxy* Proxy = GetLandscapeProxy();
	if (Proxy)
	{
		Proxy->CollisionComponents.Remove(this);
	}

	Super::DestroyComponent();
}

FBoxSphereBounds ULandscapeHeightfieldCollisionComponent::CalcBounds(const FTransform & LocalToWorld) const
{
	return CachedLocalBox.TransformBy(LocalToWorld);
}

void ULandscapeHeightfieldCollisionComponent::BeginDestroy()
{
	HeightfieldRef = NULL;
	HeightfieldGuid = FGuid();
	Super::BeginDestroy();
}

void ULandscapeMeshCollisionComponent::BeginDestroy()
{
	MeshRef = NULL;
	MeshGuid = FGuid();
	Super::BeginDestroy();
}

void ULandscapeHeightfieldCollisionComponent::RecreateCollision(bool bUpdateAddCollision/*= true*/)
{
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		HeightfieldRef = NULL;
		HeightfieldGuid = FGuid();
#if WITH_EDITOR
		if (bUpdateAddCollision)
		{
			UpdateAddCollisions();
		}
#endif

		RecreatePhysicsState();
	}
}

void ULandscapeMeshCollisionComponent::RecreateCollision(bool bUpdateAddCollision/*= true*/)
{
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		MeshRef = NULL;
		MeshGuid = FGuid();
	}

	Super::RecreateCollision(bUpdateAddCollision);
}

void ULandscapeHeightfieldCollisionComponent::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	// Necessary for background navimesh building
	FScopeLock ScopeLock(&CollisionDataSyncObject);
	CollisionHeightData.Serialize(Ar,this);
	DominantLayerData.Serialize(Ar,this);
}

void ULandscapeMeshCollisionComponent::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	CollisionXYOffsetData.Serialize(Ar,this);

	// PhysX cooking mesh data
	bool bCooked = false;
	if (Ar.UE4Ver() >= VER_UE4_ADD_COOKED_TO_LANDSCAPE)
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
#if WITH_EDITOR
		if (Ar.IsCooking())
		{
			// During cooking, always create new MeshRef
			MeshRef = GSharedMeshRefs.Add( MeshGuid, new FPhysXMeshRef(MeshGuid) );

			FMatrix LandscapeComponentMatrix = GetComponentToWorld().ToMatrixWithScale();
			bool bIsMirrored = LandscapeComponentMatrix.Determinant() < 0.f;

			FName Format = Ar.CookingTarget()->GetPhysicsFormat(NULL);
			GetCookedData(Format, bIsMirrored); // Get the data from the DDC or build it

			TArray<FName> ActualFormatsToSave;
			ActualFormatsToSave.Add(Format);
			CookedFormatData.Serialize(Ar, this, &ActualFormatsToSave);
		}
		else
#endif
		{
			CookedFormatData.Serialize(Ar, this);
		}
	}
}

#if WITH_EDITOR
void ULandscapeHeightfieldCollisionComponent::PostEditImport()
{
	Super::PostEditImport();
	// Reinitialize physics after paste
	if (CollisionSizeQuads > 0)
	{
		RecreateCollision(false);
	}
}

void ULandscapeHeightfieldCollisionComponent::PostEditUndo()
{
	Super::PostEditUndo();
	// Reinitialize physics after undo
	if (CollisionSizeQuads > 0)
	{
		RecreateCollision(false);
	}
}


void ULandscapeHeightfieldCollisionComponent::PreSave()
{
	Super::PreSave();
	if( bIncludeHoles && GIsEditor && !GetWorld()->IsPlayInEditor() && CollisionSizeQuads > 0 )
	{
		if( !bHeightFieldDataHasHole )
		{
			bHeightFieldDataHasHole = true;
			RecreateCollision(false);
		}
	}
};
#endif

void ULandscapeHeightfieldCollisionComponent::GetCollisionTriangles(TArray<FVector>& OutVertexBuffer, TArray<int32>& OutIndexBuffer) const
{
	// Necessary for background navimesh building
	{
		FScopeLock ScopeLock(&CollisionDataSyncObject);
		uint16* Heights = (uint16*)CollisionHeightData.LockReadOnly();
		int32 NumHeights = FMath::Square(CollisionSizeQuads+1);
		check(CollisionHeightData.GetElementCount()==NumHeights);

		OutVertexBuffer.Empty( FMath::Square(CollisionSizeQuads+1) );
		OutIndexBuffer.Empty( 6 * FMath::Square(CollisionSizeQuads) );

		for( int32 Y=0;Y<=CollisionSizeQuads;Y++ )
		{
			for( int32 X=0;X<=CollisionSizeQuads;X++ )
			{
				OutVertexBuffer.Add(FVector((float)X * CollisionScale, (float)Y * CollisionScale, ((float)Heights[X + Y*(CollisionSizeQuads+1)] - 32768.f) * LANDSCAPE_ZSCALE));
			}
		}
		CollisionHeightData.Unlock();
	}

	for( int32 Y=0;Y<CollisionSizeQuads;Y++ )
	{
		for( int32 X=0;X<CollisionSizeQuads;X++ )
		{
			int32 I00 = X+0 + (Y+0)*(CollisionSizeQuads+1);
			int32 I01 = X+0 + (Y+1)*(CollisionSizeQuads+1);
			int32 I10 = X+1 + (Y+0)*(CollisionSizeQuads+1);
			int32 I11 = X+1 + (Y+1)*(CollisionSizeQuads+1);

			OutIndexBuffer.Add(I00);
			OutIndexBuffer.Add(I11);
			OutIndexBuffer.Add(I10);

			OutIndexBuffer.Add(I00);
			OutIndexBuffer.Add(I01);
			OutIndexBuffer.Add(I11);
		}
	}

}

bool ULandscapeHeightfieldCollisionComponent::DoCustomNavigableGeometryExport(struct FNavigableGeometryExport* GeomExport) const
{
	TArray<FVector> VertexBuffer;
	TArray<int32> IndexBuffer;
	GetCollisionTriangles(VertexBuffer, IndexBuffer);
	GeomExport->ExportCustomMesh(VertexBuffer.GetData(), VertexBuffer.Num(), IndexBuffer.GetData(), IndexBuffer.Num(), ComponentToWorld);

	return false;
}


#if WITH_EDITOR
void ULandscapeHeightfieldCollisionComponent::PostLoad()
{
	Super::PostLoad();

	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		if (!CachedLocalBox.IsValid && CachedBoxSphereBounds_DEPRECATED.SphereRadius > 0)
		{
			ALandscapeProxy* LandscapeProxy = GetLandscapeProxy();
			USceneComponent* LandscapeRoot = LandscapeProxy->GetRootComponent();
			check(LandscapeRoot == AttachParent);

			// Component isn't attached so we can't use its LocalToWorld
			FTransform ComponentLtWTransform = FTransform(RelativeRotation, RelativeLocation, RelativeScale3D) * FTransform(LandscapeRoot->RelativeRotation, LandscapeRoot->RelativeLocation, LandscapeRoot->RelativeScale3D);

			// This is good enough. The exact box will be calculated during painting.
			CachedLocalBox = CachedBoxSphereBounds_DEPRECATED.GetBox().InverseTransformBy(ComponentLtWTransform);
		}
	}

	bHeightFieldDataHasHole = true;

	// convert from deprecated layer names to direct LayerInfo references
	ALandscapeProxy* LandscapeProxy = GetLandscapeProxy();
	if (ensure(LandscapeProxy) && ComponentLayerInfos.Num() == 0 && ComponentLayers_DEPRECATED.Num() > 0)
	{
		ComponentLayerInfos.AddUninitialized(ComponentLayers_DEPRECATED.Num());

		static const FName DataWeightmapName = FName("__DataLayer__");
		for (int32 i = 0; i < ComponentLayers_DEPRECATED.Num(); i++)
		{
			if (ComponentLayers_DEPRECATED[i] != NAME_None)
			{
				if (ComponentLayers_DEPRECATED[i] == DataWeightmapName)
				{
					ComponentLayerInfos[i] = ALandscapeProxy::DataLayer;
				}
				else
				{
					FLandscapeLayerStruct* Layer = LandscapeProxy->GetLayerInfo_Deprecated(ComponentLayers_DEPRECATED[i]);
					ComponentLayerInfos[i] = Layer ? Layer->LayerInfoObj : NULL;
				}
			}
		}

		ComponentLayers_DEPRECATED.Empty();
	}
}

void ULandscapeInfo::UpdateAllAddCollisions()
{
	for (auto It = XYtoComponentMap.CreateIterator(); It; ++It )
	{
		ULandscapeComponent* Comp = It.Value();
		if (Comp)
		{
			ULandscapeHeightfieldCollisionComponent* CollisionComp = Comp->CollisionComponent.Get();
			if (CollisionComp)
			{
				CollisionComp->UpdateAddCollisions();
			}
		}
	}
}

void ULandscapeHeightfieldCollisionComponent::UpdateAddCollisions()
{
	ULandscapeInfo* Info = GetLandscapeInfo();
	if (Info)
	{
		ALandscapeProxy* Proxy = GetLandscapeProxy();
		FIntPoint ComponentBase = GetSectionBase()/Proxy->ComponentSizeQuads;

		FIntPoint NeighborsKeys[8] = 
		{
			ComponentBase + FIntPoint(-1,-1),
			ComponentBase + FIntPoint(+0,-1),
			ComponentBase + FIntPoint(+1,-1),
			ComponentBase + FIntPoint(-1,+0),
			ComponentBase + FIntPoint(+1,+0),
			ComponentBase + FIntPoint(-1,+1),
			ComponentBase + FIntPoint(+0,+1),
			ComponentBase + FIntPoint(+1,+1)
		};

		// Search for Neighbors...
		for (int32 i = 0; i < 8; ++i)
		{
			ULandscapeComponent* Comp = Info->XYtoComponentMap.FindRef(NeighborsKeys[i]);
			if (!Comp || !Comp->CollisionComponent.IsValid())
			{
				Info->UpdateAddCollision(NeighborsKeys[i]);
			}
			else
			{
				Info->XYtoAddCollisionMap.Remove(NeighborsKeys[i]);
			}
		}
	}
}

void ULandscapeInfo::UpdateAddCollision(FIntPoint LandscapeKey)
{
	FLandscapeAddCollision* AddCollision = XYtoAddCollisionMap.Find(LandscapeKey);
	if (!AddCollision)
	{
		AddCollision = &XYtoAddCollisionMap.Add(LandscapeKey, FLandscapeAddCollision());
	}

	check(AddCollision);

	// 8 Neighbors... 
	// 0 1 2 
	// 3   4
	// 5 6 7
	FIntPoint NeighborsKeys[8] = 
	{
		LandscapeKey + FIntPoint(-1,-1),
		LandscapeKey + FIntPoint(+0,-1),
		LandscapeKey + FIntPoint(+1,-1),
		LandscapeKey + FIntPoint(-1,+0),
		LandscapeKey + FIntPoint(+1,+0),
		LandscapeKey + FIntPoint(-1,+1),
		LandscapeKey + FIntPoint(+0,+1),
		LandscapeKey + FIntPoint(+1,+1)
	};

	ULandscapeHeightfieldCollisionComponent* NeighborCollisions[8];
	// Search for Neighbors...
	for (int32 i = 0; i < 8; ++i)
	{
		ULandscapeComponent* Comp = XYtoComponentMap.FindRef(NeighborsKeys[i]);
		if (Comp)
		{
			NeighborCollisions[i] = Comp->CollisionComponent.Get();
		}
		else
		{
			NeighborCollisions[i] = NULL;
		}
	}

	uint8 CornerSet = 0;
	uint16 HeightCorner[4];

	// Corner Cases...
	if (NeighborCollisions[0])
	{
		// Necessary for background navimesh building
		FScopeLock ScopeLock(&NeighborCollisions[0]->CollisionDataSyncObject);
		uint16* Heights = (uint16*)NeighborCollisions[0]->CollisionHeightData.Lock(LOCK_READ_ONLY);
		int32 CollisionSizeVerts = NeighborCollisions[0]->CollisionSizeQuads + 1;
		HeightCorner[0] = Heights[ CollisionSizeVerts-1 + (CollisionSizeVerts-1)*CollisionSizeVerts ];
		CornerSet |= 1;
		NeighborCollisions[0]->CollisionHeightData.Unlock();
	}
	if (NeighborCollisions[2])
	{
		// Necessary for background navimesh building
		FScopeLock ScopeLock(&NeighborCollisions[2]->CollisionDataSyncObject);
		uint16* Heights = (uint16*)NeighborCollisions[2]->CollisionHeightData.Lock(LOCK_READ_ONLY);
		int32 CollisionSizeVerts = NeighborCollisions[2]->CollisionSizeQuads + 1;
		HeightCorner[1] = Heights[ (CollisionSizeVerts-1)*CollisionSizeVerts ];
		CornerSet |= 1 << 1;
		NeighborCollisions[2]->CollisionHeightData.Unlock();
	}
	if (NeighborCollisions[5])
	{
		// Necessary for background navimesh building
		FScopeLock ScopeLock(&NeighborCollisions[5]->CollisionDataSyncObject);
		uint16* Heights = (uint16*)NeighborCollisions[5]->CollisionHeightData.Lock(LOCK_READ_ONLY);
		int32 CollisionSizeVerts = NeighborCollisions[5]->CollisionSizeQuads + 1;
		HeightCorner[2] = Heights[ (CollisionSizeVerts-1) ];
		CornerSet |= 1 << 2;
		NeighborCollisions[5]->CollisionHeightData.Unlock();
	}
	if (NeighborCollisions[7])
	{
		// Necessary for background navimesh building
		FScopeLock ScopeLock(&NeighborCollisions[7]->CollisionDataSyncObject);
		uint16* Heights = (uint16*)NeighborCollisions[7]->CollisionHeightData.Lock(LOCK_READ_ONLY);
		int32 CollisionSizeVerts = NeighborCollisions[7]->CollisionSizeQuads + 1;
		HeightCorner[3] = Heights[ 0 ];
		CornerSet |= 1 << 3;
		NeighborCollisions[7]->CollisionHeightData.Unlock();
	}

	// Other cases...
	if (NeighborCollisions[1])
	{
		// Necessary for background navimesh building
		FScopeLock ScopeLock(&NeighborCollisions[1]->CollisionDataSyncObject);
		uint16* Heights = (uint16*)NeighborCollisions[1]->CollisionHeightData.Lock(LOCK_READ_ONLY);
		int32 CollisionSizeVerts = NeighborCollisions[1]->CollisionSizeQuads + 1;
		HeightCorner[0] = Heights[ (CollisionSizeVerts-1)*CollisionSizeVerts ];
		CornerSet |= 1;
		HeightCorner[1] = Heights[ CollisionSizeVerts-1 + (CollisionSizeVerts-1)*CollisionSizeVerts ];
		CornerSet |= 1 << 1;
		NeighborCollisions[1]->CollisionHeightData.Unlock();
	}
	if (NeighborCollisions[3])
	{
		// Necessary for background navimesh building
		FScopeLock ScopeLock(&NeighborCollisions[3]->CollisionDataSyncObject);
		uint16* Heights = (uint16*)NeighborCollisions[3]->CollisionHeightData.Lock(LOCK_READ_ONLY);
		int32 CollisionSizeVerts = NeighborCollisions[3]->CollisionSizeQuads + 1;
		HeightCorner[0] = Heights[ (CollisionSizeVerts-1) ];
		CornerSet |= 1;
		HeightCorner[2] = Heights[ CollisionSizeVerts-1 + (CollisionSizeVerts-1)*CollisionSizeVerts ];
		CornerSet |= 1 << 2;
		NeighborCollisions[3]->CollisionHeightData.Unlock();
	}
	if (NeighborCollisions[4])
	{
		// Necessary for background navimesh building
		FScopeLock ScopeLock(&NeighborCollisions[4]->CollisionDataSyncObject);
		uint16* Heights = (uint16*)NeighborCollisions[4]->CollisionHeightData.Lock(LOCK_READ_ONLY);
		int32 CollisionSizeVerts = NeighborCollisions[4]->CollisionSizeQuads + 1;
		HeightCorner[1] = Heights[ 0 ];
		CornerSet |= 1 << 1;
		HeightCorner[3] = Heights[ (CollisionSizeVerts-1)*CollisionSizeVerts ];
		CornerSet |= 1 << 3;
		NeighborCollisions[4]->CollisionHeightData.Unlock();
	}
	if (NeighborCollisions[6])
	{
		// Necessary for background navimesh building
		FScopeLock ScopeLock(&NeighborCollisions[6]->CollisionDataSyncObject);
		uint16* Heights = (uint16*)NeighborCollisions[6]->CollisionHeightData.Lock(LOCK_READ_ONLY);
		int32 CollisionSizeVerts = NeighborCollisions[6]->CollisionSizeQuads + 1;
		HeightCorner[2] = Heights[ 0 ];
		CornerSet |= 1 << 2;
		HeightCorner[3] = Heights[ (CollisionSizeVerts-1) ];
		CornerSet |= 1 << 3;
		NeighborCollisions[6]->CollisionHeightData.Unlock();
	}

	// Fill unset values
	// First iteration only for valid values distance 1 propagation
	// Second iteration fills left ones...
	FillCornerValues(CornerSet, HeightCorner);
	//check(CornerSet == 15);

	FIntPoint SectionBase = LandscapeKey*ComponentSizeQuads;

	// Transform Height to Vectors...
	FMatrix LtoW = GetLandscapeProxy()->LandscapeActorToWorld().ToMatrixWithScale(); 
	AddCollision->Corners[0] = LtoW.TransformPosition( FVector(SectionBase.X,						SectionBase.Y,					  LandscapeDataAccess::GetLocalHeight(HeightCorner[0])) );
	AddCollision->Corners[1] = LtoW.TransformPosition( FVector(SectionBase.X+ComponentSizeQuads,	SectionBase.Y,					  LandscapeDataAccess::GetLocalHeight(HeightCorner[1])) );
	AddCollision->Corners[2] = LtoW.TransformPosition( FVector(SectionBase.X,						SectionBase.Y+ComponentSizeQuads, LandscapeDataAccess::GetLocalHeight(HeightCorner[2])) );
	AddCollision->Corners[3] = LtoW.TransformPosition( FVector(SectionBase.X+ComponentSizeQuads,	SectionBase.Y+ComponentSizeQuads, LandscapeDataAccess::GetLocalHeight(HeightCorner[3])) );
}

void ULandscapeHeightfieldCollisionComponent::ExportCustomProperties(FOutputDevice& Out, uint32 Indent)
{
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return;
	}
	// Necessary for background navimesh building
	FScopeLock ScopeLock(&CollisionDataSyncObject);
	uint16* Heights = (uint16*)CollisionHeightData.Lock(LOCK_READ_ONLY);
	int32 NumHeights = FMath::Square(CollisionSizeQuads+1);
	check(CollisionHeightData.GetElementCount()==NumHeights);

	Out.Logf( TEXT("%sCustomProperties CollisionHeightData "), FCString::Spc(Indent));
	for( int32 i=0;i<NumHeights;i++ )
	{
		Out.Logf( TEXT("%d "), Heights[i] );
	}

	CollisionHeightData.Unlock();
	Out.Logf( TEXT("\r\n") );

	int32 NumDominantLayerSamples = DominantLayerData.GetElementCount();
	check(NumDominantLayerSamples == 0 || NumDominantLayerSamples==NumHeights);

	if( NumDominantLayerSamples > 0 )
	{
		uint8* DominantLayerSamples = (uint8*)DominantLayerData.Lock(LOCK_READ_ONLY);

		Out.Logf( TEXT("%sCustomProperties DominantLayerData "), FCString::Spc(Indent));
		for( int32 i=0;i<NumDominantLayerSamples;i++ )
		{
			Out.Logf( TEXT("%02x"), DominantLayerSamples[i] );
		}

		DominantLayerData.Unlock();
		Out.Logf( TEXT("\r\n") );
	}
}

void ULandscapeHeightfieldCollisionComponent::ImportCustomProperties(const TCHAR* SourceText, FFeedbackContext* Warn)
{
	if(FParse::Command(&SourceText,TEXT("CollisionHeightData")))
	{
		int32 NumHeights = FMath::Square(CollisionSizeQuads+1);

		// Necessary for background navimesh building
		FScopeLock ScopeLock(&CollisionDataSyncObject);
		CollisionHeightData.Lock(LOCK_READ_WRITE);
		uint16* Heights = (uint16*)CollisionHeightData.Realloc(NumHeights);
		FMemory::Memzero(Heights, sizeof(uint16)*NumHeights);

		FParse::Next(&SourceText);
		int32 i = 0;
		while( FChar::IsDigit(*SourceText) ) 
		{
			if( i < NumHeights )
			{
				Heights[i++] = FCString::Atoi(SourceText);
				while( FChar::IsDigit(*SourceText) ) 
				{
					SourceText++;
				}
			}

			FParse::Next(&SourceText);
		} 

		CollisionHeightData.Unlock();

		if( i != NumHeights )
		{
			Warn->Logf( *NSLOCTEXT( "Core", "SyntaxError", "Syntax Error" ).ToString() );
		}
	}
	else if(FParse::Command(&SourceText,TEXT("DominantLayerData")))
	{
		int32 NumDominantLayerSamples = FMath::Square(CollisionSizeQuads+1);

		DominantLayerData.Lock(LOCK_READ_WRITE);
		uint8* DominantLayerSamples = (uint8*)DominantLayerData.Realloc(NumDominantLayerSamples);
		FMemory::Memzero(DominantLayerSamples, NumDominantLayerSamples);

		FParse::Next(&SourceText);
		int32 i = 0;
		while( SourceText[0] && SourceText[1] )
		{
			if( i < NumDominantLayerSamples )
			{
				DominantLayerSamples[i++] = FParse::HexDigit(SourceText[0]) * 16 + FParse::HexDigit(SourceText[1]);
			}
			SourceText += 2;
		} 

		DominantLayerData.Unlock();

		if( i != NumDominantLayerSamples )
		{
			Warn->Logf( *NSLOCTEXT( "Core", "SyntaxError", "Syntax Error" ).ToString() );
		}
	}
}

void ULandscapeMeshCollisionComponent::ExportCustomProperties(FOutputDevice& Out, uint32 Indent)
{
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return;
	}

	Super::ExportCustomProperties(Out, Indent);

	uint16* XYOffsets = (uint16*)CollisionXYOffsetData.Lock(LOCK_READ_ONLY);
	int32 NumOffsets = FMath::Square(CollisionSizeQuads+1) * 2;
	check(CollisionXYOffsetData.GetElementCount()==NumOffsets);

	Out.Logf( TEXT("%sCustomProperties CollisionXYOffsetData "), FCString::Spc(Indent));
	for( int32 i=0;i<NumOffsets;i++ )
	{
		Out.Logf( TEXT("%d "), XYOffsets[i] );
	}

	CollisionXYOffsetData.Unlock();
	Out.Logf( TEXT("\r\n") );
}

void ULandscapeMeshCollisionComponent::ImportCustomProperties(const TCHAR* SourceText, FFeedbackContext* Warn)
{
	if(FParse::Command(&SourceText,TEXT("CollisionHeightData")))
	{
		int32 NumHeights = FMath::Square(CollisionSizeQuads+1);

		// Necessary for background navimesh building
		FScopeLock ScopeLock(&CollisionDataSyncObject);
		CollisionHeightData.Lock(LOCK_READ_WRITE);
		uint16* Heights = (uint16*)CollisionHeightData.Realloc(NumHeights);
		FMemory::Memzero(Heights, sizeof(uint16)*NumHeights);

		FParse::Next(&SourceText);
		int32 i = 0;
		while( FChar::IsDigit(*SourceText) ) 
		{
			if( i < NumHeights )
			{
				Heights[i++] = FCString::Atoi(SourceText);
				while( FChar::IsDigit(*SourceText) ) 
				{
					SourceText++;
				}
			}

			FParse::Next(&SourceText);
		} 

		CollisionHeightData.Unlock();

		if( i != NumHeights )
		{
			Warn->Logf( *NSLOCTEXT( "Core", "SyntaxError", "Syntax Error" ).ToString() );
		}
	}
	else if(FParse::Command(&SourceText,TEXT("DominantLayerData")))
	{
		int32 NumDominantLayerSamples = FMath::Square(CollisionSizeQuads+1);

		DominantLayerData.Lock(LOCK_READ_WRITE);
		uint8* DominantLayerSamples = (uint8*)DominantLayerData.Realloc(NumDominantLayerSamples);
		FMemory::Memzero(DominantLayerSamples, NumDominantLayerSamples);

		FParse::Next(&SourceText);
		int32 i = 0;
		while( SourceText[0] && SourceText[1] )
		{
			if( i < NumDominantLayerSamples )
			{
				DominantLayerSamples[i++] = FParse::HexDigit(SourceText[0]) * 16 + FParse::HexDigit(SourceText[1]);
			}
			SourceText += 2;
		} 

		DominantLayerData.Unlock();

		if( i != NumDominantLayerSamples )
		{
			Warn->Logf( *NSLOCTEXT( "Core", "SyntaxError", "Syntax Error" ).ToString() );
		}
	}
	else if (FParse::Command(&SourceText,TEXT("CollisionXYOffsetData")))
	{
		int32 NumOffsets = FMath::Square(CollisionSizeQuads+1) * 2;

		CollisionXYOffsetData.Lock(LOCK_READ_WRITE);
		uint16* Offsets = (uint16*)CollisionXYOffsetData.Realloc(NumOffsets);
		FMemory::Memzero(Offsets, sizeof(uint16)*NumOffsets);

		FParse::Next(&SourceText);
		int32 i = 0;
		while( FChar::IsDigit(*SourceText) ) 
		{
			if( i < NumOffsets )
			{
				Offsets[i++] = FCString::Atoi(SourceText);
				while( FChar::IsDigit(*SourceText) ) 
				{
					SourceText++;
				}
			}

			FParse::Next(&SourceText);
		} 

		CollisionXYOffsetData.Unlock();

		if( i != NumOffsets )
		{
			Warn->Logf( *NSLOCTEXT( "Core", "SyntaxError", "Syntax Error" ).ToString() );
		}
	}
}

ULandscapeInfo* ULandscapeHeightfieldCollisionComponent::GetLandscapeInfo(bool bSpawnNewActor /*= true*/) const
{
	if (GetLandscapeProxy())
	{
		return GetLandscapeProxy()->GetLandscapeInfo(bSpawnNewActor);
	}
	return NULL;
}

#endif // WITH_EDITOR

ALandscape* ULandscapeHeightfieldCollisionComponent::GetLandscapeActor() const
{
	ALandscapeProxy* Landscape = GetLandscapeProxy();
	if (Landscape)
	{
		return Landscape->GetLandscapeActor();
	}
	return NULL;
}

ALandscapeProxy* ULandscapeHeightfieldCollisionComponent::GetLandscapeProxy() const
{
	return CastChecked<ALandscapeProxy>(GetOuter());
}

FIntPoint ULandscapeHeightfieldCollisionComponent::GetSectionBase() const
{
	return FIntPoint(SectionBaseX, SectionBaseY);
}

void ULandscapeHeightfieldCollisionComponent::SetSectionBase(FIntPoint InSectionBase)
{
	SectionBaseX = InSectionBase.X;
	SectionBaseY = InSectionBase.Y;
}

ULandscapeHeightfieldCollisionComponent::ULandscapeHeightfieldCollisionComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	BodyInstance.bEnableCollision_DEPRECATED = true;
	SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	CastShadow = false;
	bUseAsOccluder = true;
	bAllowCullDistanceVolume = false;
	Mobility = EComponentMobility::Static;
	bIncludeHoles = false;
	bCanEverAffectNavigation = true;
	bHasCustomNavigableGeometry = EHasCustomNavigableGeometry::Yes;
	bHeightFieldDataHasHole = true;
}
