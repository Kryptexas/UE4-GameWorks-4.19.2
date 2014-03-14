// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PrimitiveSceneProxy.h"

class ENGINE_API FNavLinkRenderingProxy : public FPrimitiveSceneProxy
{
private:
	AActor* LinkOwnerActor;
	class INavLinkHostInterface* LinkOwnerHost;

public:
	struct FNavLinkDrawing
	{
		FVector Left;
		FVector Right;
		ENavLinkDirection::Type Direction;
		FColor Color;
		float SnapRadius;
	};
	struct FNavLinkSegmentDrawing
	{
		FVector LeftStart, LeftEnd;
		FVector RightStart, RightEnd;
		ENavLinkDirection::Type Direction;
		FColor Color;
		float SnapRadius;
	};

private:
	TArray<FNavLinkDrawing> OffMeshPointLinks;
	TArray<FNavLinkSegmentDrawing> OffMeshSegmentLinks;

public:
	/** Initialization constructor. */
	FNavLinkRenderingProxy(const UPrimitiveComponent* InComponent);
	virtual void DrawDynamicElements(FPrimitiveDrawInterface* PDI,const FSceneView* View) OVERRIDE;
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) OVERRIDE;
	virtual uint32 GetMemoryFootprint( void ) const OVERRIDE;
	uint32 GetAllocatedSize( void ) const;
	void StorePointLinks(const FTransform& LocalToWorld, const TArray<FNavigationLink>& LinksArray);
	void StoreSegmentLinks(const FTransform& LocalToWorld, const TArray<FNavigationSegmentLink>& LinksArray);

	/** made static to allow consistent navlinks drawing even if something is drawing links without FNavLinkRenderingProxy */
	static void DrawLinks(FPrimitiveDrawInterface* PDI, TArray<FNavLinkDrawing>& OffMeshPointLinks, TArray<FNavLinkSegmentDrawing>& OffMeshSegmentLinks, TArray<float>& StepHeights, FMaterialRenderProxy* const MeshColorInstance);
};
