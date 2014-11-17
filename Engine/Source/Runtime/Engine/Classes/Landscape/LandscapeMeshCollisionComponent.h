// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "LandscapeMeshCollisionComponent.generated.h"

UCLASS()
class ULandscapeMeshCollisionComponent : public ULandscapeHeightfieldCollisionComponent
{
	GENERATED_UCLASS_BODY()

	// Keep the possibility to share projected height field PhysX object with editor mesh collision objects...

	/** Guid used to share PhysX heightfield objects in the editor */
	UPROPERTY()
	FGuid MeshGuid;

	struct FPhysXMeshRef : public FRefCountedObject
	{
		FGuid Guid;

#if WITH_PHYSX
		/** List of PxMaterials used on this landscape */
		TArray<class physx::PxMaterial*>	UsedPhysicalMaterialArray;
		class physx::PxTriangleMesh*		RBTriangleMesh;
#if WITH_EDITOR
		class physx::PxTriangleMesh*		RBTriangleMeshEd; // Used only by landscape editor, does not have holes in it
#endif	//WITH_EDITOR
#endif	//WITH_PHYSX

		/** tors **/
		FPhysXMeshRef() 
#if WITH_PHYSX
			:	RBTriangleMesh(NULL)
#if WITH_EDITOR
			,	RBTriangleMeshEd(NULL)
#endif	//WITH_EDITOR
#endif	//WITH_PHYSX
		{}
		FPhysXMeshRef(FGuid& InGuid)
			:	Guid(InGuid)
#if WITH_PHYSX
			,	RBTriangleMesh(NULL)
#if WITH_EDITOR
			,	RBTriangleMeshEd(NULL)
#endif	//WITH_EDITOR
#endif	//WITH_PHYSX
		{}
		virtual ~FPhysXMeshRef();
	};

	/** The collision mesh values. */
	FWordBulkData								CollisionXYOffsetData; //  X, Y Offset in raw format...

	/** Physics engine version of heightfield data. */
	TRefCountPtr<struct FPhysXMeshRef>			MeshRef;

	// Begin UActorComponent interface.
	virtual void CreatePhysicsState() OVERRIDE;
	virtual void ApplyWorldOffset(const FVector& InOffset, bool bWorldShift) OVERRIDE;
	// End UActorComponent interface.

	// Begin USceneComponent interface.
	virtual void DestroyComponent() OVERRIDE;
	// End USceneComponent interface.

	// Begin UPrimitiveComponent interface
	virtual bool DoCustomNavigableGeometryExport(struct FNavigableGeometryExport* GeomExport) const OVERRIDE;
	//End UPrimitiveComponent interface

	// Begin UObject Interface.
	virtual void Serialize(FArchive& Ar) OVERRIDE;
	virtual void BeginDestroy() OVERRIDE;
#if WITH_EDITOR
	virtual void ExportCustomProperties(FOutputDevice& Out, uint32 Indent) OVERRIDE;
	virtual void ImportCustomProperties(const TCHAR* SourceText, FFeedbackContext* Warn) OVERRIDE;

	virtual bool CookCollsionData(const FName& Format, bool bUseOnlyDefMaterial, TArray<uint8>& OutCookedData, TArray<UPhysicalMaterial*>& OutMaterails) const OVERRIDE;
#endif
	// End UObject Interface.

	// Begin ULandscapeHeightfieldCollisionComponent Interface
	virtual void CreateCollisionObject() OVERRIDE;
	virtual void RecreateCollision(bool bUpdateAddCollision = true) OVERRIDE;
	// End ULandscapeHeightfieldCollisionComponent Interface
};

