// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "FunctionalTestingPrivatePCH.h"

//----------------------------------------------------------------------//
// FFTestRenderingSceneProxy
//----------------------------------------------------------------------//
class FFTestRenderingSceneProxy : public FPrimitiveSceneProxy
{
public:
	/** Initialization constructor. */
	FFTestRenderingSceneProxy(const UFuncTestRenderingComponent* InComponent)
		: FPrimitiveSceneProxy(InComponent)
	{
	}

	virtual void DrawDynamicElements(FPrimitiveDrawInterface* PDI,const FSceneView* View)
	{
	}

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View)
	{
		FPrimitiveViewRelevance Result;
		Result.bDrawRelevance = IsShown(View) && IsSelected();
		Result.bDynamicRelevance = true;
		Result.bNormalTranslucencyRelevance = IsShown(View);
		return Result;
	}

	virtual uint32 GetMemoryFootprint( void ) const { return( sizeof( *this ) + GetAllocatedSize() ); }
	uint32 GetAllocatedSize( void ) const { return FPrimitiveSceneProxy::GetAllocatedSize(); }

protected:
	AFunctionalTest* OwningTest;
};
//----------------------------------------------------------------------//
// UFuncTestRenderingComponent
//----------------------------------------------------------------------//
UFuncTestRenderingComponent::UFuncTestRenderingComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);

	AlwaysLoadOnClient = false;
	AlwaysLoadOnServer = false;
	bSelectable = false;
}

FPrimitiveSceneProxy* UFuncTestRenderingComponent::CreateSceneProxy()
{
	return new FFTestRenderingSceneProxy(this);
}

FBoxSphereBounds UFuncTestRenderingComponent::CalcBounds(const FTransform & LocalToWorld) const
{
	FBox BoundingBox = GetOwner()->GetComponentsBoundingBox();
	return FBoxSphereBounds(BoundingBox);
}
