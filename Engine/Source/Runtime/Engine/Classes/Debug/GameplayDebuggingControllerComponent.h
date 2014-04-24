// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * GameplayDebuggingComponent is used to replicate debug data from server to client(s).
 */

#pragma once
#include "GameplayDebuggingControllerComponent.generated.h"

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

UCLASS(config=Engine)
class ENGINE_API UGameplayDebuggingControllerComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(config)
	FKey DebugComponentKey;

	UPROPERTY(config)
	FKey ToggleDebugComponentKey;

	UPROPERTY(config)
	FString DebugComponentHUDClassName;

	void OnDebugAI(class UCanvas* Canvas, class APlayerController* PC);

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) OVERRIDE;
	virtual void BeginDestroy() OVERRIDE;
	virtual void InitializeComponent();
	void InitBasicFuncionality();
	void BindActivationKeys();

	APawn* GetCurrentDebugTarget() const;

	/** periodic update of navmesh data */
	void UpdateNavMeshTimer();

	virtual void OnControlPressed();
	virtual void OnControlReleased();


	virtual void SetActiveViews(uint32 InActiveViews);
	virtual uint32 GetActiveViews();
	virtual void SetActiveView(EAIDebugDrawDataView::Type NewView);
	virtual void ToggleActiveView(EAIDebugDrawDataView::Type NewView);
	virtual void CycleActiveView();
	virtual void EnableActiveView(EAIDebugDrawDataView::Type View, bool bEnable);

	FORCEINLINE bool IsViewActive(EAIDebugDrawDataView::Type View) const { return (ActiveViews & (1 << View)) != 0 || (StaticActiveViews & (1 << View)) != 0; }
	static bool ToggleStaticView(EAIDebugDrawDataView::Type View);
	FORCEINLINE static void SetEnableStaticView(EAIDebugDrawDataView::Type View, bool bEnable)
	{
		if (bEnable)
		{
			StaticActiveViews |= (1 << View);
		}
		else
		{
			StaticActiveViews &= ~(1 << View);
		}
	}

protected:

	UPROPERTY(Transient)
	class AHUD*	OnDebugAIHUD;

	UPROPERTY(Transient)
	class APawn* DebugAITargetActor;

	UPROPERTY(Transient)
	class UInputComponent* AIDebugViewInputComponent;

	virtual void StartAIDebugView();
	virtual void BeginAIDebugView();
	virtual void LockAIDebugView();
	virtual void EndAIDebugView();

	/** Sets appropriate EngineFlags.GameplayDebug flag to bNewValue. 
	 *	@param MyPC used to determine which EngineFlags instance to use. If NULL component's Owner will be used */
	void SetGameplayDebugFlag(bool bNewValue, APlayerController* PlayerController = NULL);

	bool CanToggleView();
	virtual void ToggleAIDebugView();
	virtual void ToggleAIDebugView_CycleView();
	virtual void ToggleAIDebugView_SetView0();
	virtual void ToggleAIDebugView_SetView1();
	virtual void ToggleAIDebugView_SetView2();
	virtual void ToggleAIDebugView_SetView3();
	virtual void ToggleAIDebugView_SetView4();
	virtual void ToggleAIDebugView_SetView5();
	virtual void ToggleAIDebugView_SetView6();
	virtual void ToggleAIDebugView_SetView7();
	virtual void ToggleAIDebugView_SetView8();
	virtual void ToggleAIDebugView_SetView9();

	virtual void BindAIDebugViewKeys();
	
	/** flags indicating debug channels to draw. Sums up with StaticActiveViews */
	uint32 ActiveViews;
	/** flags indicating debug channels to draw, statically accessible. Sums up with ActiveViews */
	static uint32 StaticActiveViews;

	UClass* DebugComponentHUDClass;
	float ControlKeyPressedTime;
	uint32 bWasDisplayingInfo : 1;
	uint32 bDisplayAIDebugInfo : 1;
	uint32 bWaitingForOwnersComponent : 1;
};
