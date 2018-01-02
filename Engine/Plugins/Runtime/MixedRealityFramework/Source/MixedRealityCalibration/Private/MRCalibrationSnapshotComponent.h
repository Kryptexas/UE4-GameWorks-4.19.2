// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/SceneCaptureComponent2D.h"
#include "MRCalibrationSnapshotComponent.generated.h"


/**
 *	
 */
UCLASS(ClassGroup = Rendering, Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent))
class UMRCalibrationSnapshotComponent : public USceneCaptureComponent2D
{
	GENERATED_UCLASS_BODY()

public:
	//~ USceneComponent interface
#if WITH_EDITOR
	virtual bool GetEditorPreviewInfo(float DeltaTime, FMinimalViewInfo& ViewOut) override;
#endif 

public:
	//~ USceneCaptureComponent interface
	virtual const AActor* GetViewOwner() const override;

public:
	//~ Blueprint API	

	UPROPERTY(BlueprintReadWrite, Category=SceneCapture)
	AActor* ViewOwner;
};
