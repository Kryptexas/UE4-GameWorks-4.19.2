// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "DebugRenderSceneProxy.h"
#include "EQSRenderingComponent.generated.h"

class FEQSSceneProxy : public FDebugRenderSceneProxy
{
public:
	struct FEQSItemDebugData
	{
		FString Label;
		FVector Location;
		float Score;
	};

	FEQSSceneProxy(const UPrimitiveComponent* InComponent, const FString& ViewFlagName = TEXT("DebugAI"), bool bDrawOnlyWhenSelected=true);

	virtual void RegisterDebugDrawDelgate() OVERRIDE;
	virtual void UnregisterDebugDrawDelgate() OVERRIDE;
	void DrawDebugLabels(UCanvas* Canvas, APlayerController*);
	
	//virtual void DrawStaticElements(FStaticPrimitiveDrawInterface* PDI) OVERRIDE;
	virtual void DrawDynamicElements(FPrimitiveDrawInterface* PDI, const FSceneView* View) OVERRIDE;
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) OVERRIDE;
	virtual uint32 GetMemoryFootprint( void ) const;
	uint32 GetAllocatedSize( void ) const;

private:
	FEnvQueryResult QueryResult;	
	FDebugDrawDelegate DebugTextDrawingDelegate;
	TArray<FEQSItemDebugData> DebugItems;
	TArray<FEQSItemDebugData> FailedDebugItems;
	AActor* ActorOwner;
	const class IEQSQueryResultSourceInterface* QueryDataSource;
	bool bUntestedItems;
	bool bDrawOnlyWhenSelected;
	const uint32 ViewFlagIndex;
	const FString ViewFlagName;

	static const FVector ItemDrawRadius;
};

UCLASS(HeaderGroup=Component, hidecategories=Object)
class UEQSRenderingComponent : public UPrimitiveComponent
{
	GENERATED_UCLASS_BODY()

	FString DrawFlagName;

	virtual FPrimitiveSceneProxy* CreateSceneProxy() OVERRIDE;
	virtual FBoxSphereBounds CalcBounds(const FTransform &LocalToWorld) const OVERRIDE;
	virtual void CreateRenderState_Concurrent() OVERRIDE;
	virtual void DestroyRenderState_Concurrent() OVERRIDE;
};
