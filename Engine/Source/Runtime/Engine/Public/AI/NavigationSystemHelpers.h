// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AI/Navigation/NavLinkDefinition.h"

struct FNavigableGeometryExport
{
	virtual ~FNavigableGeometryExport() {}
#if WITH_PHYSX
	virtual void ExportPxTriMesh16Bit(physx::PxTriangleMesh const * const TriMesh, const FTransform& LocalToWorld) = 0;
	virtual void ExportPxTriMesh32Bit(physx::PxTriangleMesh const * const TriMesh, const FTransform& LocalToWorld) = 0;
	virtual void ExportPxConvexMesh(physx::PxConvexMesh const * const ConvexMesh, const FTransform& LocalToWorld) = 0;
#endif // WITH_PHYSX
	virtual void ExportCustomMesh(const FVector* VertexBuffer, int32 NumVerts, const int32* IndexBuffer, int32 NumIndices, const FTransform& LocalToWorld) = 0;
	
	virtual void AddNavModifiers(const FCompositeNavModifier& Modifiers) = 0;
};


namespace NavigationHelper
{
	void GatherCollision(class UBodySetup* RigidBody, TNavStatArray<FVector>& OutVertexBuffer, TNavStatArray<int32>& OutIndexBuffer, const FTransform& ComponentToWorld = FTransform::Identity);
	void GatherCollision(class UBodySetup* RigidBody, class UNavCollision* NavCollision);

	DECLARE_DELEGATE_ThreeParams(FNavLinkProcessorDelegate, struct FCompositeNavModifier*, const class AActor*, const TArray<FNavigationLink>&);
	DECLARE_DELEGATE_ThreeParams(FNavLinkSegmentProcessorDelegate, struct FCompositeNavModifier*, const class AActor*, const TArray<FNavigationSegmentLink>&);

	/** Set new implementation of nav link processor, a function that will be
	 *	be used to process/transform links before adding them to CompositeModifier.
	 *	This function is supposed to be called once during the engine/game 
	 *	setup phase. Not intended to be toggled at runtime */
	ENGINE_API void SetNavLinkProcessorDelegate(const FNavLinkProcessorDelegate& NewDelegate);
	ENGINE_API void SetNavLinkSegmentProcessorDelegate(const FNavLinkSegmentProcessorDelegate& NewDelegate);

	/** called to do any necessary processing on NavLinks and put results in CompositeModifier */
	ENGINE_API void ProcessNavLinkAndAppend(struct FCompositeNavModifier* OUT CompositeModifier, const class AActor* Actor, const TArray<FNavigationLink>& IN NavLinks);

	/** called to do any necessary processing on NavLinks and put results in CompositeModifier */
	ENGINE_API void ProcessNavLinkSegmentAndAppend(struct FCompositeNavModifier* OUT CompositeModifier, const class AActor* Actor, const TArray<FNavigationSegmentLink>& IN NavLinks);

	ENGINE_API void DefaultNavLinkProcessorImpl(struct FCompositeNavModifier* OUT CompositeModifier, const class AActor* Actor, const TArray<FNavigationLink>& IN NavLinks);

	ENGINE_API void DefaultNavLinkSegmentProcessorImpl(struct FCompositeNavModifier* OUT CompositeModifier, const class AActor* Actor, const TArray<FNavigationSegmentLink>& IN NavLinks);
}
