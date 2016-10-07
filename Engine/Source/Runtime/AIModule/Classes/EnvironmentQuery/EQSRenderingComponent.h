// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "DebugRenderSceneProxy.h"
#include "Debug/DebugDrawService.h"
#include "EnvQueryTypes.h"
#include "EnvQueryDebugHelpers.h"
#include "Components/PrimitiveComponent.h"
#include "EQSRenderingComponent.generated.h"

class IEQSQueryResultSourceInterface;
struct FEnvQueryInstance;

class AIMODULE_API FEQSSceneProxy : public FDebugRenderSceneProxy
{
	friend class FEQSRenderingDebugDrawDelegateHelper;
public:
	FEQSSceneProxy(const UPrimitiveComponent* InComponent, const FString& ViewFlagName = TEXT("DebugAI"));
	FEQSSceneProxy(const UPrimitiveComponent* InComponent, const FString& ViewFlagName, const TArray<FSphere>& Spheres, const TArray<FText3d>& Texts);
	
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override;

#if  USE_EQS_DEBUGGER 
	static void CollectEQSData(const UPrimitiveComponent* InComponent, const IEQSQueryResultSourceInterface* QueryDataSource, TArray<FSphere>& Spheres, TArray<FText3d>& Texts, TArray<EQSDebug::FDebugHelper>& DebugItems);
	static void CollectEQSData(const FEnvQueryResult* ResultItems, const FEnvQueryInstance* QueryInstance, float HighlightRangePct, bool ShouldDrawFailedItems, TArray<FSphere>& Spheres, TArray<FText3d>& Texts, TArray<EQSDebug::FDebugHelper>& DebugItems);
#endif
private:
	FEnvQueryResult QueryResult;	
	// can be 0
	AActor* ActorOwner;
	const IEQSQueryResultSourceInterface* QueryDataSource;
	uint32 bDrawOnlyWhenSelected : 1;

	static const FVector ItemDrawRadius;

	bool SafeIsActorSelected() const;
};

#if  USE_EQS_DEBUGGER
class FEQSRenderingDebugDrawDelegateHelper : public FDebugDrawDelegateHelper
{
	typedef FDebugDrawDelegateHelper Super;

public:
	FEQSRenderingDebugDrawDelegateHelper()
		: ActorOwner(NULL)
		, QueryDataSource(NULL)
		, bDrawOnlyWhenSelected(false)
	{
	}

	virtual void InitDelegateHelper(const FDebugRenderSceneProxy* InSceneProxy) override
	{
		check(0);
	}

	void InitDelegateHelper(const FEQSSceneProxy* InSceneProxy)
	{
		Super::InitDelegateHelper(InSceneProxy);

		ActorOwner = InSceneProxy->ActorOwner;
		QueryDataSource = InSceneProxy->QueryDataSource;
		bDrawOnlyWhenSelected = InSceneProxy->bDrawOnlyWhenSelected;
	}

protected:
	AIMODULE_API virtual void DrawDebugLabels(UCanvas* Canvas, APlayerController*) override;

private:
	// can be 0
	AActor* ActorOwner;
	const IEQSQueryResultSourceInterface* QueryDataSource;
	uint32 bDrawOnlyWhenSelected : 1;
};
#endif

UCLASS(hidecategories=Object)
class AIMODULE_API UEQSRenderingComponent : public UPrimitiveComponent
{
	GENERATED_UCLASS_BODY()

	FString DrawFlagName;
	uint32 bDrawOnlyWhenSelected : 1;

#if  USE_EQS_DEBUGGER || ENABLE_VISUAL_LOG
	EQSDebug::FQueryData DebugData;
#endif

	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual FBoxSphereBounds CalcBounds(const FTransform &LocalToWorld) const override;
	virtual void CreateRenderState_Concurrent() override;
	virtual void DestroyRenderState_Concurrent() override;

#if  USE_EQS_DEBUGGER
	FEQSRenderingDebugDrawDelegateHelper EQSRenderingDebugDrawDelegateHelper;
#endif
};
