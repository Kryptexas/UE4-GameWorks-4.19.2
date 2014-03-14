// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BodySetup.generated.h"

struct FDynamicMeshVertex;

namespace physx
{
	class PxConvexMesh;
	class PxTriangleMesh;
	class PxRigidActor;
}

UENUM()
enum ECollisionTraceFlag
{
	// default, we keep simple/complex separate for each test
	CTF_UseDefault UMETA(DisplayName="Default"),
	// use simple collision for complex collision test
	CTF_UseSimpleAsComplex UMETA(DisplayName="Use Simple Collision As Complex"),
	// use complex collision (per poly) for simple collision test
	CTF_UseComplexAsSimple UMETA(DisplayName="Use Complex Collision As Simple"),
	CTF_MAX,
};

UENUM()
enum EPhysicsType
{
	// follow owner option
	PhysType_Default UMETA(DisplayName="Default"),	
	// Do not follow owner, but fix 
	PhysType_Fixed	UMETA(DisplayName="Fixed"),		
	// Do not follow owner, but unfix 
	PhysType_Unfixed	UMETA(DisplayName="Unfixed")	
};

UENUM()
namespace EBodyCollisionResponse
{
	enum Type
	{
		BodyCollision_Enabled UMETA(DisplayName="Enabled"), 
		BodyCollision_Disabled UMETA(DisplayName="Disabled")//, 
		//BodyCollision_Custom UMETA(DisplayName="Custom")
	};
}

/** One convex hull, used for simplified collision. */
USTRUCT()
struct FKConvexElem
{
	GENERATED_USTRUCT_BODY()

	/** Array of indices that make up the convex hull. */
	UPROPERTY()
	TArray<FVector> VertexData;

	/** Bounding box of this convex hull. */
	UPROPERTY()
	FBox ElemBox;

	/** Transform of this element */
	UPROPERTY()
	FTransform Transform;

		/** Convex mesh for this body, created from cooked data in CreatePhysicsMeshes */
		physx::PxConvexMesh*   ConvexMesh;	

		/** Convex mesh for this body, flipped across X, created from cooked data in CreatePhysicsMeshes */
		physx::PxConvexMesh*   ConvexMeshNegX;

		FKConvexElem() 
		: ElemBox(0)
		, Transform(FTransform::Identity)
		, ConvexMesh(NULL)
		, ConvexMeshNegX(NULL)
		{}

		ENGINE_API void	DrawElemWire(class FPrimitiveDrawInterface* PDI, const FTransform& ElemTM, const FVector& Scale3D, const FColor Color);
		void	AddCachedSolidConvexGeom(TArray<FDynamicMeshVertex>& VertexBuffer, TArray<int32>& IndexBuffer, const FColor VertexColor);

		/** Reset the hull to empty all arrays */
		ENGINE_API void	Reset();

		/** Updates internal ElemBox based on current value of VertexData */
		ENGINE_API void	UpdateElemBox();

		/** Calculate a bounding box for this convex element with the specified transform and scale */
		FBox	CalcAABB(const FTransform& BoneTM, const FVector& Scale3D);		

		/** Utility for creating a convex hull from a set of planes. Will reset current state of this elem. */
		bool	HullFromPlanes(const TArray<FPlane>& InPlanes, const TArray<FVector>& SnapVerts);

		/** Returns the volume of this element */
		float GetVolume(const FVector& Scale) const;

		FTransform GetTransform() const
		{
			return Transform;
		};

		void SetTransform( const FTransform& InTransform )
		{
			Transform = InTransform;
		}

		friend FArchive& operator<<(FArchive& Ar,FKConvexElem& Elem);
	
};

/** Sphere shape used for collision */
USTRUCT()
struct FKSphereElem
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FMatrix TM_DEPRECATED;

	UPROPERTY(Category=KSphereElem, VisibleAnywhere)
	FVector Center;

	UPROPERTY(Category=KSphereElem, VisibleAnywhere)
	float Radius;

	FKSphereElem() 
	: Center( FVector::ZeroVector )
	, Radius(1)
	{

	}

	FKSphereElem( float r ) 
	: Center( FVector::ZeroVector )
	, Radius(r)
	{

	}

	void Serialize( const FArchive& Ar );

	friend bool operator==( const FKSphereElem& LHS, const FKSphereElem& RHS )
	{
		return ( LHS.Center == RHS.Center &&
			LHS.Radius == RHS.Radius );
	}

	// Utility function that builds an FTransform from the current data
	FTransform GetTransform() const
	{
		return FTransform( Center );
	};

	FORCEINLINE float GetVolume(const FVector& Scale) const { return 1.3333f * PI * FMath::Pow(Radius * Scale.GetMin(), 3); }
	
	ENGINE_API void	DrawElemWire(class FPrimitiveDrawInterface* PDI, const FTransform& ElemTM, float Scale, const FColor Color);
	ENGINE_API void	DrawElemSolid(class FPrimitiveDrawInterface* PDI, const FTransform& ElemTM, float Scale, const FMaterialRenderProxy* MaterialRenderProxy);
	FBox CalcAABB(const FTransform& BoneTM, float Scale);
};

/** Box shape used for collision */
USTRUCT()
struct FKBoxElem
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FMatrix TM_DEPRECATED;

	UPROPERTY(Category=KBoxElem, VisibleAnywhere)
	FVector Center;

	UPROPERTY(Category=KBoxElem, VisibleAnywhere)
	FQuat Orientation;

	UPROPERTY(Category=KBoxElem, VisibleAnywhere)
	float X;

	UPROPERTY(Category=KBoxElem, VisibleAnywhere)
	float Y;

	/** length (not radius) */
	UPROPERTY(Category=KBoxElem, VisibleAnywhere)
	float Z;


	FKBoxElem()
	: Center( FVector::ZeroVector )
	, Orientation( FQuat::Identity )
	, X(1), Y(1), Z(1)
	{

	}

	FKBoxElem( float s )
	: Center( FVector::ZeroVector )
	, Orientation( FQuat::Identity )
	, X(s), Y(s), Z(s)
	{

	}

	FKBoxElem( float InX, float InY, float InZ ) 
	: Center( FVector::ZeroVector )
	, Orientation( FQuat::Identity )
	, X(InX), Y(InY), Z(InZ)

	{

	}

	void Serialize( const FArchive& Ar );

	friend bool operator==( const FKBoxElem& LHS, const FKBoxElem& RHS )
	{
		return ( LHS.Center == RHS.Center &&
			LHS.Orientation == RHS.Orientation &&
			LHS.X == RHS.X &&
			LHS.Y == RHS.Y &&
			LHS.Z == RHS.Z );
	};

	// Utility function that builds an FTransform from the current data
	FTransform GetTransform() const
	{
		return FTransform( Orientation, Center );
	};

	void SetTransform( const FTransform& InTransform )
	{
		Orientation = InTransform.GetRotation();
		Center = InTransform.GetLocation();
	}

	FORCEINLINE float GetVolume(const FVector& Scale) const { float MinScale = Scale.GetMin(); return (X * MinScale) * (Y * MinScale) * (Z * MinScale); }

	ENGINE_API void	DrawElemWire(class FPrimitiveDrawInterface* PDI, const FTransform& ElemTM, float Scale, const FColor Color);
	ENGINE_API void	DrawElemSolid(class FPrimitiveDrawInterface* PDI, const FTransform& ElemTM, float Scale, const FMaterialRenderProxy* MaterialRenderProxy);
	FBox CalcAABB(const FTransform& BoneTM, float Scale);
};

/** Capsule shape used for collision */
USTRUCT()
struct FKSphylElem
{
	GENERATED_USTRUCT_BODY()

	/** The transform assumes the sphyl axis points down Z. */
	UPROPERTY()
	FMatrix TM_DEPRECATED;

	UPROPERTY(Category=KBoxElem, VisibleAnywhere)
	FVector Center;

	UPROPERTY(Category=KBoxElem, VisibleAnywhere)
	FQuat Orientation;

	UPROPERTY(Category=KSphylElem, VisibleAnywhere)
	float Radius;

	/** This is of line-segment ie. add Radius to both ends to find total length. */
	UPROPERTY(Category=KSphylElem, VisibleAnywhere)
	float Length;

	FKSphylElem()
	: Center( FVector::ZeroVector )
	, Orientation( FQuat::Identity )
	, Radius(1), Length(1)

	{

	}

	FKSphylElem( float InRadius, float InLength )
	: Center( FVector::ZeroVector )
	, Orientation( FQuat::Identity )
	, Radius(InRadius), Length(InLength)
	{

	}

	void Serialize( const FArchive& Ar );

	friend bool operator==( const FKSphylElem& LHS, const FKSphylElem& RHS )
	{
		return ( LHS.Center == RHS.Center &&
			LHS.Orientation == RHS.Orientation &&
			LHS.Radius == RHS.Radius &&
			LHS.Length == RHS.Length );
	};

	// Utility function that builds an FTransform from the current data
	FTransform GetTransform() const
	{
		return FTransform( Orientation, Center );
	};

	void SetTransform( const FTransform& InTransform )
	{
		Orientation = InTransform.GetRotation();
		Center = InTransform.GetLocation();
	}

	FORCEINLINE float GetVolume(const FVector& Scale) const { float ScaledRadius = Radius * Scale.GetMin(); return PI * FMath::Square(ScaledRadius) * ( 1.3333f * ScaledRadius + (Length * Scale.GetMin())); }

	ENGINE_API void	DrawElemWire(class FPrimitiveDrawInterface* PDI, const FTransform& ElemTM, float Scale, const FColor Color);
	ENGINE_API void	DrawElemSolid(class FPrimitiveDrawInterface* PDI, const FTransform& ElemTM, float Scale, const FMaterialRenderProxy* MaterialRenderProxy);
	FBox CalcAABB(const FTransform& BoneTM, float Scale);
};

/** Container for an aggregate of collision shapes */
USTRUCT()
struct ENGINE_API FKAggregateGeom
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, editfixedsize, Category=KAggregateGeom)
	TArray<struct FKSphereElem> SphereElems;

	UPROPERTY(EditAnywhere, editfixedsize, Category=KAggregateGeom)
	TArray<struct FKBoxElem> BoxElems;

	UPROPERTY(EditAnywhere, editfixedsize, Category=KAggregateGeom)
	TArray<struct FKSphylElem> SphylElems;

	UPROPERTY(EditAnywhere, editfixedsize, Category=KAggregateGeom)
	TArray<struct FKConvexElem> ConvexElems;

	class FKConvexGeomRenderInfo* RenderInfo;

	FKAggregateGeom() 
	: RenderInfo(NULL)
	{}
	int32 GetElementCount() const
	{
		return SphereElems.Num() + SphylElems.Num() + BoxElems.Num() + ConvexElems.Num();
	}

	int32 GetElementCount(int32 Type) const;

	void EmptyElements()
	{
		BoxElems.Empty();
		ConvexElems.Empty();
		SphylElems.Empty();
		SphereElems.Empty();

		FreeRenderInfo();
	}

	

	void Serialize( const FArchive& Ar );

	void DrawAggGeom(class FPrimitiveDrawInterface* PDI, const FTransform& Transform, const FColor Color, const FMaterialRenderProxy* MatInst, bool bPerHullColor, bool bDrawSolid);

	/** Release the RenderInfo (if its there) and safely clean up any resources. Call on the game thread. */
	void FreeRenderInfo();

	FBox CalcAABB(const FTransform& Transform);

	/**
		* Calculates a tight box-sphere bounds for the aggregate geometry; this is more expensive than CalcAABB
		* (tight meaning the sphere may be smaller than would be required to encompass the AABB, but all individual components lie within both the box and the sphere)
		*
		* @param Output The output box-sphere bounds calculated for this set of aggregate geometry
		*	@param LocalToWorld Transform
		*/
	void CalcBoxSphereBounds(FBoxSphereBounds& Output, const FTransform& LocalToWorld);

	/** Returns the volume of this element */
	float GetVolume(const FVector& Scale) const;
};

UCLASS(hidecategories=Object, MinimalAPI, dependson=BodyInstance)
class UBodySetup : public UObject
{
	GENERATED_UCLASS_BODY()

	/** Simplified collision representation of this  */
	UPROPERTY()
	struct FKAggregateGeom AggGeom;

	/** Used in the PhysicsAsset case. Associates this Body with Bone in a skeletal mesh. */
	UPROPERTY(Category=BodySetup, VisibleAnywhere)
	FName BoneName;

	/** 
	 *	If Unfixed it will use physics. If fixed, it will use kinematic. Default will inherit from OwnerComponent's behavior.
	 */
	UPROPERTY(EditAnywhere, Category=Physics)
	TEnumAsByte<EPhysicsType> PhysicsType;

	/** 
	 *	If true (and bEnableFullAnimWeightBodies in SkelMeshComp is true), the physics of this bone will always be blended into the skeletal mesh, regardless of what PhysicsWeight of the SkelMeshComp is. 
	 *	This is useful for bones that should always be physics, even when blending physics in and out for hit reactions (eg cloth or pony-tails).
	 */
	UPROPERTY()
	uint32 bAlwaysFullAnimWeight_DEPRECATED:1;

	/** 
	 *	Should this BodySetup be considered for the bounding box of the PhysicsAsset (and hence SkeletalMeshComponent).
	 *	There is a speed improvement from having less BodySetups processed each frame when updating the bounds.
	 */
	UPROPERTY(EditAnywhere, Category=BodySetup)
	uint32 bConsiderForBounds:1;

	/** 
	 *	If true, the physics of this mesh (only affects static meshes) will always contain ALL elements from the mesh - not just the ones enabled for collision. 
	 *	This is useful for forcing high detail collisions using the entire render mesh.
	 */
	UPROPERTY(Transient)
	uint32 bMeshCollideAll:1;

	/**	Should we generate data necessary to support collision on normal (non-mirrored) versions of this body. */
	UPROPERTY()
	uint32 bGenerateNonMirroredCollision:1;

	/** 
	 *	Should we generate data necessary to support collision on mirrored versions of this mesh. 
	 *	This halves the collision data size for this mesh, but disables collision on mirrored instances of the body.
	 */
	UPROPERTY()
	uint32 bGenerateMirroredCollision:1;

	/** Physical material to use for simple collision on this body. Encodes information about density, friction etc. */
	UPROPERTY(EditAnywhere, Category=Physics, meta=(DisplayName="Simple Collision Physical Material"))
	class UPhysicalMaterial* PhysMaterial;

	/** Collision Type for this body. This eventually changes response to collision to others **/
	UPROPERTY(EditAnywhere, Category=Collision)
	TEnumAsByte<enum EBodyCollisionResponse::Type> CollisionReponse;

	/** Collision Trace behavior - by default, it will keep simple(convex)/complex(per-poly) separate **/
	UPROPERTY(EditAnywhere, Category=Collision, meta=(DisplayName = "Collision Complexity"))
	TEnumAsByte<enum ECollisionTraceFlag> CollisionTraceFlag;

	/** Default properties of the body instance, copied into objects on instantiation, was URB_BodyInstance */
	UPROPERTY(EditAnywhere, Category=Collision, meta=(FullyExpand = "true"))
	struct FBodyInstance DefaultInstance;

	/** Custom walkable slope setting for this body. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category=Physics)
	struct FWalkableSlopeOverride WalkableSlopeOverride;

	/** BuildScale for this body setup (static mesh settings define this value) */
	UPROPERTY()
	float BuildScale;
public:
	/** GUID used to uniquely identify this setup so it can be found in the DDC */
	FGuid						BodySetupGuid;

	/** Cooked physics data for each format */
	FFormatContainer			CookedFormatData;

	/** Physics triangle mesh, created from cooked data in CreatePhysicsMeshes */
	physx::PxTriangleMesh*		TriMesh;

	/** Physics triangle mesh, flipped across X, created from cooked data in CreatePhysicsMeshes */
	physx::PxTriangleMesh*		TriMeshNegX;

	/** Flag used to know if we have created the physics convex and tri meshes from the cooked data yet */
	bool						bCreatedPhysicsMeshes;

	/** Indicates whether this setup has any cooked collision data. */
	bool						bHasCookedCollisionData;

	// Begin UObject interface.
	virtual void Serialize(FArchive& Ar) OVERRIDE;
	virtual void BeginDestroy() OVERRIDE;
	virtual void FinishDestroy() OVERRIDE;
	virtual void PostLoad() OVERRIDE;
	virtual void PostInitProperties() OVERRIDE;
	virtual SIZE_T GetResourceSize(EResourceSizeMode::Type Mode) OVERRIDE;
	// End UObject interface.

	//
	// UBodySetup interface.
	//
	ENGINE_API void			CopyBodyPropertiesFrom(class UBodySetup* FromSetup);

	/** Add collision shapes from another body setup to this one */
	ENGINE_API void			AddCollisionFrom(class UBodySetup* FromSetup);


	/** Create Physics meshes (ConvexMeshes, TriMesh & TriMeshNegX) from cooked data */
	/** Release Physics meshes (ConvexMeshes, TriMesh & TriMeshNegX). Must be called before the BodySetup is destroyed */
	ENGINE_API void			CreatePhysicsMeshes();


	/** Release Physics meshes (ConvexMeshes, TriMesh & TriMeshNegX) */
	ENGINE_API void			ClearPhysicsMeshes();

	/** Calculates the mass. You can pass in the component where additional information is pulled from ( Scale, PhysMaterialOverride ) */
	ENGINE_API float		CalculateMass(const UPrimitiveComponent* Component = NULL);

	/** Returns the physics material used for this body. If none, specified, returns the default engine material. */
	class UPhysicalMaterial* GetPhysMaterial() const;

#if WITH_EDITOR
	/** Clear all simple collision */
	ENGINE_API void			RemoveSimpleCollision();

	/** 
	 * Rescales simple collision geometry.  Note you must recreate physics meshes after this 
	 *
	 * @param UniformScale	The uniform scale to apply to the geometry
	 */
	ENGINE_API void			RescaleSimpleCollision( float UniformScale );

	/** Invalidate physics data */
	ENGINE_API void			InvalidatePhysicsData();	

	/**
	 * Converts a UModel to a set of convex hulls for simplified collision.  Any convex elements already in
	 * this BodySetup will be destroyed.  WARNING: the input model can have no single polygon or
	 * set of coplanar polygons which merge to more than FPoly::MAX_VERTICES vertices.
	 *
	 * @param		InModel					The input BSP.
	 * @param		bRemoveExisting			If true, clears any pre-existing collision
	 * @return								true on success, false on failure because of vertex count overflow.
	 */
	ENGINE_API void			CreateFromModel(class UModel* InModel, bool bRemoveExisting);
#endif // WITH_EDITOR

	/**
	 * Given a format name returns its cooked data.
	 *
	 * @param Format Physics format name.
	 * @return Cooked data or NULL of the data was not found.
	 */
	FByteBulkData* GetCookedData(FName Format);

#if WITH_PHYSX
	/** 
	 *   Add the shapes defined by this body setup to the supplied PxRigidBody. 
	 */
	void                    AddShapesToRigidActor(physx::PxRigidActor* PDestActor, FVector& Scale3D);
#endif // WITH_PHYSX

};



