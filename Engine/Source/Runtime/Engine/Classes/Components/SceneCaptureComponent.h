// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "SceneCaptureComponent.generated.h"

	// -> will be exported to EngineDecalClasses.h
UCLASS(hidecategories=(abstract, Collision, Object, Physics, SceneComponent, Mobility), HeaderGroup=Decal, MinimalAPI)
class USceneCaptureComponent : public USceneComponent
{
	GENERATED_UCLASS_BODY()

	/** Whether to update the capture's contents every frame.  If disabled, the component will render once on load and then only when moved. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=SceneCapture)
	bool bCaptureEveryFrame;

	/** if > 0, sets a maximum render distance override.  Can be used to cull distant objects from a reflection if the reflecting plane is in an enclosed area like a hallway or room */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=SceneCapture, meta=(UIMin = "100", UIMax = "10000"))
	float MaxViewDistanceOverride;

public:

};

