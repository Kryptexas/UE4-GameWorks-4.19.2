// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "LandscapeHeightfieldCollisionComponent.generated.h"

UCLASS(HeaderGroup=Terrain, MinimalAPI)
class ULandscapeHeightfieldCollisionComponent : public UPrimitiveComponent
{
	GENERATED_UCLASS_BODY()

	/** DEPRECATED List of layers painted on this component. Matches the WeightmapLayerAllocations array in the LandscapeComponent. */
	UPROPERTY()
	TArray<FName> ComponentLayers_DEPRECATED;

	/** List of layers painted on this component. Matches the WeightmapLayerAllocations array in the LandscapeComponent. */
	UPROPERTY()
	TArray<ULandscapeLayerInfoObject*> ComponentLayerInfos;

	/** Offset of component in landscape quads */
	UPROPERTY()
	int32 SectionBaseX;

	UPROPERTY()
	int32 SectionBaseY;

	/** Size of component in collision quads */
	UPROPERTY()
	int32 CollisionSizeQuads;

	/** Collision scale: (ComponentSizeQuads) / (CollisionSizeQuads) */
	UPROPERTY()
	float CollisionScale;

	/** The flags for each collision quad. See ECollisionQuadFlags. */
	UPROPERTY()
	TArray<uint8> CollisionQuadFlags;

	/** Guid used to share PhysX heightfield objects in the editor */
	UPROPERTY()
	FGuid HeightfieldGuid;

	/** Cached bounds, created at heightmap update time */
	UPROPERTY()
	FBoxSphereBounds CachedBoxSphereBounds_DEPRECATED;

	/** Cached local-space bounding box, created at heightmap update time */
	UPROPERTY()
	FBox CachedLocalBox;

	UPROPERTY()
	uint32 bIncludeHoles:1;

	UPROPERTY()
	uint32 bHeightFieldDataHasHole:1;

	/** Reference to render component */
	UPROPERTY()
	TLazyObjectPtr<class ULandscapeComponent> RenderComponent;

	mutable FCriticalSection CollisionDataSyncObject;

	struct FPhysXHeightfieldRef : public FRefCountedObject
	{
		FGuid Guid;

#if WITH_PHYSX
		/** List of PxMaterials used on this landscape */
		TArray<class physx::PxMaterial*>	UsedPhysicalMaterialArray;
		class physx::PxHeightField*			RBHeightfield;
#endif	//WITH_PHYSX

		/** tors **/
		FPhysXHeightfieldRef() 
#if WITH_PHYSX
			:	RBHeightfield(NULL)
#endif	//WITH_PHYSX
		{}
		FPhysXHeightfieldRef(FGuid& InGuid)
			:	Guid(InGuid)
#if WITH_PHYSX
			,	RBHeightfield(NULL)
#endif	//WITH_PHYSX
		{}
		virtual ~FPhysXHeightfieldRef();
	};

	/** The collision height values. */
	FWordBulkData								CollisionHeightData;

	/** Indices into the ComponentLayers array for the per-vertex dominant layer. */
	FByteBulkData								DominantLayerData;

	/** Physics engine version of heightfield data. */
	TRefCountPtr<struct FPhysXHeightfieldRef>	HeightfieldRef;

	enum ECollisionQuadFlags
	{
		QF_PhysicalMaterialMask = 63,	// Mask value for the physical material index, stored in the lower 6 bits.
		QF_EdgeTurned = 64,				// This quad's diagonal has been turned.
		QF_NoCollision = 128,			// This quad has no collision.
	};

	// Begin UPrimitiveComponent interface
	virtual bool DoCustomNavigableGeometryExport(struct FNavigableGeometryExport* GeomExport) const OVERRIDE;
	//End UPrimitiveComponent interface

	// Begin UActorComponent interface.
	virtual void CreatePhysicsState() OVERRIDE;
	virtual void ApplyWorldOffset(const FVector& InOffset, bool bWorldShift) OVERRIDE;
	// End UActorComponent interface.

	// Begin USceneComponent interface.
	virtual void DestroyComponent() OVERRIDE;
	virtual FBoxSphereBounds CalcBounds(const FTransform &BoundTransform) const OVERRIDE;

	virtual ECollisionEnabled::Type GetCollisionEnabled() const OVERRIDE;
	virtual ECollisionResponse GetCollisionResponseToChannel(ECollisionChannel Channel) const OVERRIDE;
	virtual ECollisionChannel GetCollisionObjectType() const OVERRIDE;
	virtual const FCollisionResponseContainer& GetCollisionResponseToChannels() const OVERRIDE;
	// End USceneComponent interface.

	// Begin UObject Interface.
	virtual void Serialize(FArchive& Ar) OVERRIDE;
	virtual void BeginDestroy() OVERRIDE;
#if WITH_EDITOR
	virtual void ExportCustomProperties(FOutputDevice& Out, uint32 Indent) OVERRIDE;
	virtual void ImportCustomProperties(const TCHAR* SourceText, FFeedbackContext* Warn) OVERRIDE;
	virtual void PostEditImport() OVERRIDE;
	virtual void PostEditUndo() OVERRIDE;
	virtual void PreSave() OVERRIDE;
	virtual void PostLoad() OVERRIDE;
	// End UObject Interface.

	// Update Collision object for add LandscapeComponent tool
	ENGINE_API void UpdateAddCollisions();

	// @todo document
	class ULandscapeInfo* GetLandscapeInfo(bool bSpawnNewActor = true) const;
#endif

	/** Return a list of collision triangles in local space */
	void GetCollisionTriangles(TArray<FVector>& OutVertexBuffer, TArray<int32>& OutIndexBuffer) const;

	/** Return the landscape actor associated with this component. */
	class ALandscape* GetLandscapeActor() const;
	ENGINE_API class ALandscapeProxy* GetLandscapeProxy() const;

	/** @return Component section base as FIntPoint */
	ENGINE_API FIntPoint GetSectionBase() const; 

	/** @param InSectionBase new section base for a component */
	ENGINE_API void SetSectionBase(FIntPoint InSectionBase);

	/** Recreate heightfield and restart physics */
	ENGINE_API virtual void RecreateCollision(bool bUpdateAddCollision = true);

	/** Modify a sub-region of the PhysX heightfield. Note that this does not update the physical material */
	void UpdateHeightfieldRegion(int32 ComponentX1, int32 ComponentY1, int32 ComponentX2, int32 ComponentY2);

};



