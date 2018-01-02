// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Misc/Guid.h"
#include "Misc/CoreStats.h"
#include "Templates/RefCounting.h"
#include "Components/SceneComponent.h"
#include "RenderCommandFence.h"
#include "ReflectionCaptureComponent.generated.h"

class FReflectionCaptureProxy;
class UBillboardComponent;

UENUM()
enum class EReflectionSourceType : uint8
{
	/** Construct the reflection source from the captured scene*/
	CapturedScene,
	/** Construct the reflection source from the specified cubemap. */
	SpecifiedCubemap,
};

// -> will be exported to EngineDecalClasses.h
UCLASS(abstract, hidecategories=(Collision, Object, Physics, SceneComponent, Activation, "Components|Activation", Mobility), MinimalAPI)
class UReflectionCaptureComponent : public USceneComponent
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	UBillboardComponent* CaptureOffsetComponent;

	/** Indicates where to get the reflection source from. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=ReflectionCapture)
	EReflectionSourceType ReflectionSourceType;

	/** Cubemap to use for reflection if ReflectionSourceType is set to RS_SpecifiedCubemap. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=ReflectionCapture)
	class UTextureCube* Cubemap;

	/** Angle to rotate the source cubemap when SourceType is set to SLS_SpecifiedCubemap. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=ReflectionCapture, meta = (UIMin = "0", UIMax = "360"))
	float SourceCubemapAngle;

	/** A brightness control to scale the captured scene's reflection intensity. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=ReflectionCapture, meta=(UIMin = ".5", UIMax = "4"))
	float Brightness;
	
	/** World space offset to apply before capturing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=ReflectionCapture, AdvancedDisplay)
	FVector CaptureOffset;

	/** Guid for map build data */
	UPROPERTY()
	FGuid MapBuildDataId;

	/** The rendering thread's mirror of this reflection capture. */
	FReflectionCaptureProxy* SceneProxy;

	/** Callback to create the rendering thread mirror. */
	ENGINE_API FReflectionCaptureProxy* CreateSceneProxy();

	/** Called to update the preview shapes when something they are dependent on has changed. */
	virtual void UpdatePreviewShape();

	/** Adds the capture to the capture queue processed by UpdateReflectionCaptureContents. */
	ENGINE_API void MarkDirtyForRecaptureOrUpload();

	/** Generates a new MapBuildDataId and adds the capture to the capture queue processed by UpdateReflectionCaptureContents. */
	ENGINE_API void MarkDirtyForRecapture();

	/** Marks this component has having been recaptured. */
	void SetCaptureCompleted() { bNeedsRecaptureOrUpload = false; }

	/** Gets the radius that bounds the shape's influence, used for culling. */
	virtual float GetInfluenceBoundingRadius() const PURE_VIRTUAL(UReflectionCaptureComponent::GetInfluenceBoundingRadius,return 0;);

	/** Called each tick to recapture and queued reflection captures. */
	ENGINE_API static void UpdateReflectionCaptureContents(UWorld* WorldToUpdate, const TCHAR* CaptureReason = nullptr, bool bVerifyOnlyCapturing = false);

	ENGINE_API class FReflectionCaptureMapBuildData* GetMapBuildData() const;

	virtual void PropagateLightingScenarioChange() override;

	ENGINE_API static int32 GetReflectionCaptureSize();

	//~ Begin UActorComponent Interface
	virtual void CreateRenderState_Concurrent() override;
	virtual void DestroyRenderState_Concurrent() override;
	virtual void SendRenderTransform_Concurrent() override;
	virtual void OnRegister() override;
	virtual void InvalidateLightingCacheDetailed(bool bInvalidateBuildEnqueuedLighting, bool bTranslationOnly) override;
	//~ End UActorComponent Interface

	//~ Begin UObject Interface
	virtual void PostInitProperties() override;	
	virtual void Serialize(FArchive& Ar) override;
#if WITH_EDITOR
	virtual bool CanEditChange(const UProperty* Property) const override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PreFeatureLevelChange(ERHIFeatureLevel::Type PendingFeatureLevel) override;
#endif // WITH_EDITOR
	virtual void BeginDestroy() override;
	virtual bool IsReadyForFinishDestroy() override;
	virtual void FinishDestroy() override;
	//~ End UObject Interface

private:

	/** Whether the reflection capture needs to re-capture the scene. */
	bool bNeedsRecaptureOrUpload;

	/** Cubemap texture resource used for rendering with the encoded HDR values. */
	class FReflectionTextureCubeResource* EncodedHDRCubemapTexture;

	/** Temporary encoded data used to preview feature level changes in the editor, or running ES2 uncooked. */
	TArray<uint8> TemporaryEncodedHDRCapturedData;

	/** Fence used to track progress of releasing resources on the rendering thread. */
	FRenderCommandFence ReleaseResourcesFence;

	/** 
	 * List of reflection captures that need to be recaptured.
	 * These have to be queued because we can only render the scene to update captures at certain points, after the level has loaded.
	 * This queue should be in the UWorld or the FSceneInterface, but those are not available yet in PostLoad.
	 */
	static TArray<UReflectionCaptureComponent*> ReflectionCapturesToUpdate;

	/** 
	 * List of reflection captures that need to be recaptured because they were dirty on load.
	 * These have to be queued because we can only render the scene to update captures at certain points, after the level has loaded.
	 * This queue should be in the UWorld or the FSceneInterface, but those are not available yet in PostLoad.
	 */
	static TArray<UReflectionCaptureComponent*> ReflectionCapturesToUpdateForLoad;
	static FCriticalSection ReflectionCapturesToUpdateForLoadLock;

	//void UpdateDerivedData(FReflectionCaptureFullHDR* NewDerivedData);
	void SerializeLegacyData(FArchive& Ar);

	friend class FReflectionCaptureProxy;
};

ENGINE_API extern void GenerateEncodedHDRData(const TArray<uint8>& FullHDRData, int32 CubemapSize, float Brightness, TArray<uint8>& OutEncodedHDRData);