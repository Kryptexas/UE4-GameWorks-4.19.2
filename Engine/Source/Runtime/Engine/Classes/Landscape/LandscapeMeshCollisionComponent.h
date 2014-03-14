// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "LandscapeMeshCollisionComponent.generated.h"

UCLASS(HeaderGroup=Terrain)
class ULandscapeMeshCollisionComponent : public ULandscapeHeightfieldCollisionComponent, public IInterface_CollisionDataProvider
{
	GENERATED_UCLASS_BODY()

	// Keep the possibility to share projected height field PhysX object with editor mesh collision objects...

	/** Guid used to share PhysX heightfield objects in the editor */
	UPROPERTY()
	FGuid MeshGuid;

	/** Cooked physics data for each format */
	FFormatContainer			CookedFormatData;

	struct FPhysXMeshRef : public FRefCountedObject
	{
		FGuid Guid;

#if WITH_PHYSX
		/** List of PxMaterials used on this landscape */
		TArray<class physx::PxMaterial*>	UsedPhysicalMaterialArray;
		class physx::PxTriangleMesh*		RBTriangleMesh;
#endif	//WITH_PHYSX

		/** tors **/
		FPhysXMeshRef() 
#if WITH_PHYSX
			:	RBTriangleMesh(NULL)
#endif	//WITH_PHYSX
		{}
		FPhysXMeshRef(FGuid& InGuid)
			:	Guid(InGuid)
#if WITH_PHYSX
			,	RBTriangleMesh(NULL)
#endif	//WITH_PHYSX
		{}
		virtual ~FPhysXMeshRef();
	};

	/** The collision mesh values. */
	FWordBulkData								CollisionXYOffsetData; //  X, Y Offset in raw format...

	/** Physics engine version of heightfield data. */
	TRefCountPtr<struct FPhysXMeshRef>			MeshRef;

	TArray<int32> UpdateMaterials();

	// Begin IInterface_CollisionDataProvider Interface
	virtual bool GetPhysicsTriMeshData(struct FTriMeshCollisionData* CollisionData, bool InUseAllTriData) OVERRIDE;
	virtual bool ContainsPhysicsTriMeshData(bool InUseAllTriData) const OVERRIDE;
	// End IInterface_CollisionDataProvider Interface

	// Begin UActorComponent interface.
	virtual void CreatePhysicsState() OVERRIDE;
	virtual void ApplyWorldOffset(const FVector& InOffset, bool bWorldShift) OVERRIDE;
	// End UActorComponent interface.

	// Begin USceneComponent interface.
	virtual void DestroyComponent() OVERRIDE;
	// End USceneComponent interface.

	// Begin UObject Interface.
	virtual void Serialize(FArchive& Ar) OVERRIDE;
	virtual void BeginDestroy() OVERRIDE;
#if WITH_EDITOR
	virtual void ExportCustomProperties(FOutputDevice& Out, uint32 Indent) OVERRIDE;
	virtual void ImportCustomProperties(const TCHAR* SourceText, FFeedbackContext* Warn) OVERRIDE;
#endif
	// End UObject Interface.

	// Begin ULandscapeHeightfieldCollisionComponent Interface
	virtual void RecreateCollision(bool bUpdateAddCollision = true) OVERRIDE;

	FByteBulkData* GetCookedData(FName Format, bool bIsMirrored);
	// End ULandscapeHeightfieldCollisionComponent Interface
};

