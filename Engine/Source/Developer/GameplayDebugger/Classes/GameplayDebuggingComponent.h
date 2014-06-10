// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * GameplayDebuggingComponent is used to replicate debug data from server to client(s).
 */

#pragma once
#include "Components/PrimitiveComponent.h"
#include "DebugRenderSceneProxy.h"
#include "GameplayDebuggingControllerComponent.h"
#include "GameplayDebuggingComponent.generated.h"

#define WITH_EQS 0

struct FDebugContext;

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
		TArray<FDebugRenderSceneProxy::FSphere> SolidSpheres;
		TArray<FDebugRenderSceneProxy::FText3d> Texts;
		int32 NumValidItems;
	};
}

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnDebuggingTargetChanged, class AActor* /*Owner of debugging component*/, bool /*is being debugged now*/);

UCLASS(config=Engine)
class GAMEPLAYDEBUGGER_API UGameplayDebuggingComponent : public UPrimitiveComponent//, public IEQSQueryResultSourceInterface
{
	GENERATED_UCLASS_BODY()

	friend class AGameplayDebuggingHUDComponent;

	UPROPERTY(globalconfig)
	FString DebugComponentClassName;

	UPROPERTY(Replicated)
	bool bIsSelectedForDebugging;

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
	
#if WITH_EQS
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

	UFUNCTION()
	virtual void OnRep_UpdateEQS();
#endif // WITH_EQS

	virtual bool GetComponentClassCanReplicate() const override{ return true; }

	UFUNCTION()
	virtual void OnRep_UpdateNavmesh();

	UFUNCTION(exec)
	void ServerReplicateData(uint32 InMessage, uint32 DataView);

	UFUNCTION(reliable, server, WithValidation)
	void ServerCollectNavmeshData(FVector_NetQuantize10 TargetLocation);

	UFUNCTION(reliable, server, WithValidation)
	void ServerDiscardNavmeshData();

	void PrepareNavMeshData(struct FNavMeshSceneProxyData*) const;

	virtual void Activate(bool bReset=false) override;
	virtual void Deactivate() override;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

	virtual void EnableDebugDraw(bool bEnable, bool InFocusedComponent = false);

	FORCEINLINE bool IsSelected() const { return bIsSelectedForDebugging; }
	/** Will broadcast information that this component is (no longer) being "observed" */
	void SelectForDebugging(bool bNewStatus);

	bool ShouldReplicateData(EAIDebugDrawDataView::Type InView) const { return true; }
	virtual void CollectDataToReplicate(bool bCollectExtendedData);

	//=============================================================================
	// controller related stuff
	//=============================================================================
	UPROPERTY(Replicated)
	AActor* TargetActor;
	
	UFUNCTION(reliable, server, WithValidation)
	void ServerEnableTargetSelection(bool bEnable);

	void SetActorToDebug(AActor* Actor);
	FORCEINLINE AActor* GetSelectedActor() const
	{
		return TargetActor;
	}

#if WITH_EQS
	//=============================================================================
	// EQS debugging
	//=============================================================================
	uint32 bEnableClientEQSSceneProxy : 1;

	void EnableClientEQSSceneProxy(bool bEnable) { bEnableClientEQSSceneProxy = bEnable;  MarkRenderStateDirty(); }
	bool IsClientEQSSceneProxyEnabled() { return bEnableClientEQSSceneProxy; }

	// IEQSQueryResultSourceInterface start
	virtual const struct FEnvQueryResult* GetQueryResult() const override;
	virtual const struct FEnvQueryInstance* GetQueryInstance() const  override;

	virtual bool GetShouldDebugDrawLabels() const override { return bDrawEQSLabels; }
	virtual bool GetShouldDrawFailedItems() const override{ return bDrawEQSFailedItems; }
	// IEQSQueryResultSourceInterface end
protected:
	virtual void CollectEQSData();
public:
#endif //WITH_EQS

	//=============================================================================
	// Rendering
	//=============================================================================
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual FBoxSphereBounds CalcBounds(const FTransform &LocalToWorld) const override;
	virtual void CreateRenderState_Concurrent() override;
	virtual void DestroyRenderState_Concurrent() override;

protected:
	void SelectTargetToDebug(FDebugContext& Context);

	//APlayerController* PlayerOwner;

protected:
	virtual void CollectPathData();
	virtual void CollectBasicData();
	virtual void CollectBehaviorTreeData();

	FNavPathWeakPtr CurrentPath;

	uint32 bEnabledTargetSelection : 1;
#if WITH_EDITOR
	uint32 bWasSelectedInEditor : 1;
#endif

	float NextTargrtSelectionTime;
	/** navmesh data passed to rendering component */
	FBox NavMeshBounds;

	TWeakObjectPtr<APlayerController> PlayerOwner;

public:
	static FName DefaultComponentName;
	static FOnDebuggingTargetChanged OnDebuggingTargetChangedDelegate;
};
