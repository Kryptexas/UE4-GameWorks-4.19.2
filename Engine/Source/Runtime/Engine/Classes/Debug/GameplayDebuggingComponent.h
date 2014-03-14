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
		GameView6,
		GameView7,
		GameView8,
		GameView9,
		NavMesh,
		EditorDebugAIFlag
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

USTRUCT()
struct FOffMeshLinkRenderData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FVector Left;

	UPROPERTY()
	FVector Right;

	UPROPERTY()
	uint8	AreaID;

	UPROPERTY()
	uint8	Direction;

	UPROPERTY()
	uint8	ValidEnds;

	UPROPERTY()
	float	Radius;

	UPROPERTY()
	FColor	Color;
};

USTRUCT()
struct FNavMeshRenderData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FColor NavMeshColor;

	UPROPERTY()
	TArray<FVector> CoordBuffer;

	UPROPERTY()
	TArray<int32> IndexBuffer;
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnDebuggingTargetChanged, class AActor* /*Owner of debugging component*/, bool /*is being debugged now*/);

UCLASS(HeaderGroup=Component)
class ENGINE_API UGameplayDebuggingComponent : public UPrimitiveComponent, public IEQSQueryResultSourceInterface
{
	GENERATED_UCLASS_BODY()

	friend class AGameplayDebuggingComponentHUD;

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

	UPROPERTY(ReplicatedUsing=OnRep_MarkRenderStateDirty)
	FBox NavMeshBoundingBox;

	UPROPERTY(ReplicatedUsing=OnRep_MarkRenderStateDirty)
	FNavMeshRenderData AllNavMeshAreas[RECAST_MAX_AREAS];

	UPROPERTY(ReplicatedUsing=OnRep_MarkRenderStateDirty)
	TArray<FVector> NavMeshEdgeVerts;

	UPROPERTY(ReplicatedUsing=OnRep_MarkRenderStateDirty)
	TArray<FVector> TileEdgeVerts;

	UPROPERTY(ReplicatedUsing=OnRep_MarkRenderStateDirty)
	TArray<int32> OffMeshSegmentAreas;

	UPROPERTY(ReplicatedUsing=OnRep_MarkRenderStateDirty)
	TArray<FOffMeshLinkRenderData> OffMeshLinks;
			
	/** flags indicating debug channels to draw. Sums up with StaticActiveViews */
	uint32 ActiveViews;

	float EQSTimestamp;
	TSharedPtr<FEnvQueryInstance> EQSQueryInstance;
	uint32 bDrawEQSLabels:1;
	uint32 bDrawEQSFailedItems:1;

	UFUNCTION()
	virtual void OnRep_MarkRenderStateDirty();

	UFUNCTION(reliable, server, WithValidation)
	void ServerReplicateData( EDebugComponentMessage::Type InMessage, EAIDebugDrawDataView::Type DataView );

	UFUNCTION(reliable, server, WithValidation)
	void ServerCollectNavmeshData();
	
	void GatherDataForProxy();
	void GatherData(struct FNavMeshSceneProxyData*) const;

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

	FORCEINLINE static void ToggleStaticView(EAIDebugDrawDataView::Type View) { StaticActiveViews ^= 1 << View; }
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
	bool bEnabledTargetSelection;

protected:
	virtual void CollectPathData();
	virtual void CollectBasicData();
	virtual void CollectBehaviorTreeData();
	virtual void CollectEQSData();

	virtual FString GetKeyboardDesc();

	FNavPathWeakPtr CurrentPath;

	uint32 bIsSelectedForDebugging : 1;
#if WITH_EDITOR
	bool bWasSelectedInEditor;
#endif

	TSharedPtr<struct FNavMeshSceneProxyData, ESPMode::ThreadSafe>	NavmeshData;

private:
	/** flags indicating debug channels to draw, statically accessible. Sums up with ActiveViews */
	static uint32 StaticActiveViews;

public:
	static FName DefaultComponentName;
	static FOnDebuggingTargetChanged OnDebuggingTargetChangedDelegate;
	static const uint32 ShowFlagIndex;
};
