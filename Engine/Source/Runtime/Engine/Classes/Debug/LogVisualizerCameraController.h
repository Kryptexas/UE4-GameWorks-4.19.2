// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "LogVisualizerCameraController.generated.h"

UCLASS(config=Input)
class ALogVisualizerCameraController : public ADebugCameraController
{
public:
	DECLARE_DELEGATE_OneParam(FActorSelectedDelegate, class AActor*);
	DECLARE_DELEGATE_OneParam(FLogEntryIterationDelegate, int32);

	GENERATED_UCLASS_BODY()

	UPROPERTY()
	class AActor* PickedActor;

	virtual void PostInitializeComponents() OVERRIDE;

	ENGINE_API static ALogVisualizerCameraController* EnableCamera(UWorld* InWorld);
	ENGINE_API static void DisableCamera(UWorld* InWorld);
	ENGINE_API static bool IsEnabled(UWorld* InWorld);

	virtual void Select( FHitResult const& Hit ) OVERRIDE;

	void ShowNextEntry();
	void ShowPrevEntry();

	static TWeakObjectPtr<ALogVisualizerCameraController> Instance;

	FActorSelectedDelegate OnActorSelected;
	FLogEntryIterationDelegate OnIterateLogEntries;

protected:
	virtual void SetupInputComponent() OVERRIDE;
};
