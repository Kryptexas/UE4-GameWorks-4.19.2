// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * GameplayDebuggingComponent is used to replicate debug data from server to client(s).
 */

#pragma once
#include "GameplayDebuggingComponent.generated.h"

UENUM()
namespace EAIDebugDrawDataView
{
	enum Type
	{
		Empty,
		OverHead,
		Basic,
		BehaviorTree,
		EQS,
		Perception,
		GameView1,
		GameView2,
		GameView3,
		GameView4,
		GameView5,
		NavMesh,
		EditorDebugAIFlag,
		MAX UMETA(Hidden)
	};
}

UENUM()
namespace EDebugComponentMessage
{
	enum Type
	{
		EnableExtendedView,
		DisableExtendedView, 
		ActivateReplication, 
		DeactivateReplilcation, 
		CycleReplicationView,
		ToggleReplicationView,
	};
}

struct ENGINE_API FDebugCategoryView
{
	FString Desc;
	TEnumAsByte<EAIDebugDrawDataView::Type> View;

	FDebugCategoryView() {}
	FDebugCategoryView(EAIDebugDrawDataView::Type InView, const FString& Description) : Desc(Description), View(InView) {}
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnDebuggingTargetChanged, class AActor* /*Owner of debugging component*/, bool /*is being debugged now*/);

UCLASS(HeaderGroup=Component)
class ENGINE_API UGameplayDebuggingComponent : public UPrimitiveComponent, public IEQSQueryResultSourceInterface
{
	GENERATED_UCLASS_BODY()

	friend class AGameplayDebuggingHUDComponent;

	UPROPERTY(Replicated)
	int32 ActivationCounter;

	UPROPERTY(Replicated)
	int32 ShowExtendedInformatiomCounter;

	UPROPERTY(Replicated)
	FString ControllerName;

	UPROPERTY(Replicated)
	FString PawnName;

	UPROPERTY(Replicated)
	FString PawnClass;

	UPROPERTY(Replicated)
	FString DebugIcon;

	UPROPERTY(Replicated)
	FString BrainComponentName;

	/** Begin path replication data */
	UPROPERTY(Replicated)
	TArray<FVector> PathPoints;

	UPROPERTY(Replicated)
	FString PathErrorString;
	/** End path replication data*/
	
	UPROPERTY(ReplicatedUsing=OnRep_UpdateNavmesh)
	TArray<uint8> NavmeshRepData;

	/** flags indicating debug channels to draw. Sums up with StaticActiveViews */
	uint32 ActiveViews;

	float EQSTimestamp;
	TSharedPtr<FEnvQueryInstance> EQSQueryInstance;
	uint32 bDrawEQSLabels:1;
	uint32 bDrawEQSFailedItems:1;

	UFUNCTION()
	virtual void OnRep_UpdateNavmesh();

	UFUNCTION(reliable, server, WithValidation)
	void ServerReplicateData( EDebugComponentMessage::Type InMessage, EAIDebugDrawDataView::Type DataView );

	UFUNCTION(reliable, server, WithValidation)
	void ServerCollectNavmeshData(FVector_NetQuantize10 TargetLocation);

	UFUNCTION(reliable, server, WithValidation)
	void ServerDiscardNavmeshData();

	void PrepareNavMeshData(struct FNavMeshSceneProxyData*) const;

	virtual void Activate(bool bReset=false) OVERRIDE;
	virtual void Deactivate() OVERRIDE;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) OVERRIDE;

	virtual void EnableDebugDraw(bool bEnable, bool InFocusedComponent = false);

	virtual void SetActiveViews( uint32 InActiveViews );
	virtual uint32 GetActiveViews();
	virtual void SetActiveView( EAIDebugDrawDataView::Type NewView );
	virtual void ToggleActiveView( EAIDebugDrawDataView::Type NewView );
	virtual void CycleActiveView();
	virtual void EnableActiveView( EAIDebugDrawDataView::Type View, bool bEnable );

	FORCEINLINE bool IsViewActive(EAIDebugDrawDataView::Type View) const { return (ActiveViews & (1 << View)) != 0 || (StaticActiveViews & (1 << View)) != 0; }

	FORCEINLINE bool IsSelected() const { return bIsSelectedForDebugging; }
	/** Will broadcast information that this component is (no longer) being "observed" */
	void SelectForDebugging(bool bNewStatus);

	static bool ToggleStaticView(EAIDebugDrawDataView::Type View);
	FORCEINLINE static void SetEnableStaticView(EAIDebugDrawDataView::Type View, bool bEnable) 
	{
		if (bEnable)
		{
			StaticActiveViews|= (1 << View);
		}
		else
		{
			StaticActiveViews &= ~(1 << View);
		}
	}

	//=============================================================================
	// client side debugging
	//=============================================================================
	void OnDebugAI(class UCanvas* Canvas, class APlayerController* PC);

	virtual void CollectDataToReplicate();
	virtual bool LogGameSpecificBugIt(FOutputDevice& OutputFile) {return true;}

	//=============================================================================
	// controller related stuff
	//=============================================================================
	UPROPERTY(Replicated)
	APawn* TargetActor;

	UFUNCTION(reliable, server, WithValidation)
	void ServerEnableTargetSelection(bool bEnable);

	//=============================================================================
	// EQS debugging
	//=============================================================================
	// IEQSQueryResultSourceInterface start
	virtual const struct FEnvQueryResult* GetQueryResult() const OVERRIDE;
	virtual const struct FEnvQueryInstance* GetQueryInstance() const  OVERRIDE;

	virtual bool GetShouldDebugDrawLabels() const OVERRIDE { return bDrawEQSLabels; }
	virtual bool GetShouldDrawFailedItems() const OVERRIDE{ return bDrawEQSFailedItems; }
	// IEQSQueryResultSourceInterface end

	//=============================================================================
	// Rendering
	//=============================================================================
	virtual FPrimitiveSceneProxy* CreateSceneProxy() OVERRIDE;
	virtual FBoxSphereBounds CalcBounds(const FTransform &LocalToWorld) const OVERRIDE;
	virtual void CreateRenderState_Concurrent() OVERRIDE;
	virtual void DestroyRenderState_Concurrent() OVERRIDE;

protected:
	void SelectTargetToDebug();

	APlayerController* PlayerOwner;

protected:
	virtual void CollectPathData();
	virtual void CollectBasicData();
	virtual void CollectBehaviorTreeData();
	virtual void CollectEQSData();

	virtual void GetKeyboardDesc(TArray<FDebugCategoryView>& Categories);

	FNavPathWeakPtr CurrentPath;

	uint32 bEnabledTargetSelection : 1;
	uint32 bIsSelectedForDebugging : 1;
#if WITH_EDITOR
	uint32 bWasSelectedInEditor : 1;
#endif

	/** navmesh data passed to rendering component */
	TSharedPtr<struct FNavMeshSceneProxyData, ESPMode::ThreadSafe> NavmeshRenderData;

private:
	/** flags indicating debug channels to draw, statically accessible. Sums up with ActiveViews */
	static uint32 StaticActiveViews;

public:
	static FName DefaultComponentName;
	static FOnDebuggingTargetChanged OnDebuggingTargetChangedDelegate;
	static const uint32 ShowFlagIndex;
};
