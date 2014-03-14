// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "NavMeshRenderingComponent.generated.h"

UCLASS(HeaderGroup=Component, hidecategories=Object, editinlinenew)
class UNavMeshRenderingComponent : public UPrimitiveComponent
{
	GENERATED_UCLASS_BODY()

	// Begin UPrimitiveComponent Interface
	virtual FPrimitiveSceneProxy* CreateSceneProxy() OVERRIDE;
	// End UPrimitiveComponent Interface

	// Begin UActorComponent Interface
	virtual void CreateRenderState_Concurrent() OVERRIDE;
	virtual void DestroyRenderState_Concurrent() OVERRIDE;
	// End UActorComponent Interface

	// Begin USceneComponent Interface
	virtual FBoxSphereBounds CalcBounds(const FTransform &LocalToWorld) const OVERRIDE;
	// End USceneComponent Interface

	void GatherData(struct FNavMeshSceneProxyData*) const;
	void GatherDataForProxy();
protected:
	TSharedPtr<struct FNavMeshSceneProxyData, ESPMode::ThreadSafe>	ProxyData;
};
