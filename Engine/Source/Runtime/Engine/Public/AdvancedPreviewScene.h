// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PrettyPreviewScene.h: Pretty preview scene definitions.
=============================================================================*/

#pragma once

#if WITH_EDITOR

#include "PreviewScene.h"

#include "TickableEditorObject.h"
#include "EditorSupportDelegates.h"

struct FPreviewSceneProfile;

class USphereReflectionCaptureComponent;
class USkyLightComponent;
class UStaticMeshComponent;
class UMaterialInstanceConstant;
class UPostProcessComponent;
class UAssetViewerSettings;
class UEditorPerProjectUserSettings;

class ENGINE_API FAdvancedPreviewScene : public FPreviewScene, public FTickableEditorObject
{
public:
	FAdvancedPreviewScene(ConstructionValues CVS, float InFloorOffset = 0.0f);

	void UpdateScene(FPreviewSceneProfile& Profile, bool bUpdateSkyLight = true, bool bUpdateEnvironment = true, bool bUpdatePostProcessing = true, bool bUpdateDirectionalLight = true);

	void SetFloorOffset(const float InFloorOffset);
	void SetProfileIndex(const int32 InProfileIndex);

	/* Begin FTickableEditorObject */
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;
	/* End FTickableEditorObject */

	const bool HandleViewportInput(FViewport* InViewport, int32 ControllerId, FKey Key, float Delta, float DeltaTime, int32 NumSamples, bool bGamepad);
	const bool HandleInputKey(FViewport* InViewport, int32 ControllerId, FKey Key, EInputEvent Event, float AmountDepressed, bool Gamepad);

	const float GetSkyRotation() const;
	void SetSkyRotation(const float SkyRotation);	

	const UStaticMeshComponent* GetFloorMeshComponent() const;
protected:
	USkyLightComponent* SkyLightComponent;
	UStaticMeshComponent* SkyComponent;
	UMaterialInstanceConstant* InstancedSkyMaterial;
	UPostProcessComponent* PostProcessComponent;
	UStaticMeshComponent* FloorMeshComponent;
	UAssetViewerSettings* DefaultSettings;
	bool bRotateLighting;

	float CurrentRotationSpeed;
	float PreviousRotation;

	int32 CurrentProfileIndex;
};

#endif // WITH_EDITOR
