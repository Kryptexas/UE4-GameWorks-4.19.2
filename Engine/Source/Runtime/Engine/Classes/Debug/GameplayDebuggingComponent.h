// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * GameplayDebuggingComponent is used to replicate debug data from server to client(s).
 */

#pragma once
#include "GameplayDebuggingControllerComponent.h"
#include "GameplayDebuggingComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogGDT, Display, All);

UENUM()
namespace EDebugComponentMessage
{
	enum Type
	{
		EnableExtendedView,
		DisableExtendedView, 
		ActivateReplication,
		DeactivateReplilcation,
		ActivateDataView,
		DeactivateDataView,
		SetMultipleDataViews,
	};
}

struct ENGINE_API FDebugCategoryView
{
	FString Desc;
	TEnumAsByte<EAIDebugDrawDataView::Type> View;

	FDebugCategoryView() {}
	FDebugCategoryView(EAIDebugDrawDataView::Type InView, const FString& Description) : Desc(Description), View(InView) {}
};

namespace EQSDebug
{
	struct FItemData
	{
		FString Desc;
		int32 ItemIdx;
		float TotalScore;

		TArray<float> TestValues;
		TArray<float> TestScores;
	};

	struct FTestData
	{
		FString ShortName;
		FString Detailed;
	};

	struct FQueryData
	{
		TArray<FItemData> Items;
		TArray<FTestData> Tests;
		TArray<FEQSSceneProxy::FSphere> SolidSpheres;
		TArray<FEQSSceneProxy::FText3d> Texts;
		int32 NumValidItems;
	};
}

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnDebuggingTargetChanged, class AActor* /*Owner of debugging component*/, bool /*is being debugged now*/);

UCLASS()
class ENGINE_API UGameplayDebuggingComponent : public UPrimitiveComponent, public IEQSQueryResultSourceInterface
{
	GENERATED_UCLASS_BODY()

	friend class AGameplayDebuggingHUDComponent;

	UPROPERTY(Replicated)
	int32 ActivationCounter;

	UPROPERTY(Replicated)
	int32 ShowExtendedInformatiomCounter;

	UPROPERTY(Replicated)
	TArray<int32> ReplicateViewDataCounters;

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
	
	UPROPERTY(ReplicatedUsing = OnRep_UpdateNavmesh)
	TArray<uint8> NavmeshRepData;
	
	/** Begin EQS replication data */
	UPROPERTY(Replicated)
	float EQSTimestamp;
	
	UPROPERTY(Replicated)
	FString EQSName;
	
	UPROPERTY(Replicated)
	int32 EQSId;

	UPROPERTY(ReplicatedUsing = OnRep_UpdateEQS)
	TArray<uint8> EQSRepData;
	
	/** local EQS debug data, decoded from EQSRepData blob */
	EQSDebug::FQueryData EQSLocalData;	
	/** End EQS replication data */


	TSharedPtr<FEnvQueryInstance> CachedQueryInstance;
	uint32 bDrawEQSLabels:1;
	uint32 bDrawEQSFailedItems : 1;

	virtual bool GetComponentClassCanReplicate() const OVERRIDE{ return true; }

	UFUNCTION()
	virtual void OnRep_UpdateEQS();

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

	FORCEINLINE bool IsSelected() const { return bIsSelectedForDebugging; }
	/** Will broadcast information that this component is (no longer) being "observed" */
	void SelectForDebugging(bool bNewStatus);

	bool ShouldReplicateData(EAIDebugDrawDataView::Type InView) const { return ReplicateViewDataCounters[InView] > 0; }
	//=============================================================================
	// client side debugging
	//=============================================================================
	void OnDebugAI(class UCanvas* Canvas, class APlayerController* PC);

	virtual void CollectDataToReplicate();

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
	uint32 bEnableClientEQSSceneProxy : 1;

	void EnableClientEQSSceneProxy(bool bEnable) { bEnableClientEQSSceneProxy = bEnable;  MarkRenderStateDirty(); }
	bool IsClientEQSSceneProxyEnabled() { return bEnableClientEQSSceneProxy; }

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

	float NextTargrtSelectionTime;
	/** navmesh data passed to rendering component */
	FBox NavMeshBounds;

public:
	static FName DefaultComponentName;
	static FOnDebuggingTargetChanged OnDebuggingTargetChangedDelegate;
	static const uint32 ShowFlagIndex;
};
