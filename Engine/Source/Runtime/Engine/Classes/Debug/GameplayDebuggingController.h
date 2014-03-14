// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * GameplayDebuggingComponent is used to replicate debug data from server to client(s).
 */

#pragma once
#include "GameplayDebuggingController.generated.h"

UCLASS(config=Engine, HeaderGroup=Component)
class ENGINE_API UGameplayDebuggingController : public UActorComponent
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(config)
	FKey DebugComponentKey;

	UPROPERTY(config)
	FKey ToggleDebugComponentKey;

	UPROPERTY(config)
	FKey BugItDebugComponentKey;

	UPROPERTY(config)
	FString DebugComponentHUDClassName;

	void OnDebugAI(class UCanvas* Canvas, class APlayerController* PC);

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) OVERRIDE;
	virtual void BeginDestroy() OVERRIDE;
	virtual void InitializeComponent();

	APawn* GetCurrentDebugTarget() const;

protected:

	UPROPERTY(Transient)
	class AHUD*	OnDebugAIHUD;

	UPROPERTY(Transient)
	class AHUD*	OryginalHUD;

	UPROPERTY(Transient)
	class APawn* DebugAITargetActor;

	UPROPERTY(Transient)
	class UInputComponent* AIDebugViewInputComponent;

	virtual void StartAIDebugView();
	virtual void BeginAIDebugView(bool bJustBugIt = false);
	virtual void LockAIDebugView();
	virtual void EndAIDebugView();

	/** Sets appropriate EngineFlags.GameplayDebug flag to bNewValue. 
	 *	@param MyPC used to determine which EngineFlags instance to use. If NULL component's Owner will be used */
	void SetGameplayDebugFlag(bool bNewValue, APlayerController* PlayerController = NULL);

	virtual void ToggleAIDebugView();
	virtual void BugIt();
	virtual void TakeScreenShot();
	virtual void OnScreenshotCaptured(int32 Width, int32 Height, const TArray<FColor>& Bitmap);

	virtual void ToggleAIDebugView_CycleView();
	virtual void ToggleAIDebugView_SetView1();
	virtual void ToggleAIDebugView_SetView2();
	virtual void ToggleAIDebugView_SetView3();
	virtual void ToggleAIDebugView_SetView4();
	virtual void ToggleAIDebugView_SetView5();
	virtual void ToggleAIDebugView_SetView6();
	virtual void ToggleAIDebugView_SetView7();
	virtual void ToggleAIDebugView_SetView8();
	virtual void ToggleAIDebugView_SetView9();
	virtual void ToggleAIDebugView_SetView10();
	virtual void ToggleAIDebugView_SetView11();
	virtual void ToggleAIDebugView_SetView12();
	virtual void ToggleAIDebugView_SetView13();

	virtual void BindAIDebugViewKeys();

	void LogOutBugItGoToLogFile( const FString& InScreenShotDesc, const FString& InGoString, const FString& InLocString );


	UClass* DebugComponentHUDClass;
	uint32 DebugComponent_LastActiveViews;
	int32 CurrentQuickBugNum;
	uint32 bDisplayAIDebugInfo : 1;
	uint32 bWaitingForOwnersComponent : 1;
	uint32 bJustBugIt : 1;
};
