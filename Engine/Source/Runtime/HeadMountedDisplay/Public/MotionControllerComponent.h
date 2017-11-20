// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Components/PrimitiveComponent.h"
#include "SceneViewExtension.h"
#include "IMotionController.h"
#include "LateUpdateManager.h"
#include "IIdentifiableXRDevice.h" // for FXRDeviceId
#include "MotionControllerComponent.generated.h"

class FPrimitiveSceneInfo;
class FRHICommandListImmediate;
class FSceneView;
class FSceneViewFamily;

UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent), ClassGroup = MotionController)
class HEADMOUNTEDDISPLAY_API UMotionControllerComponent : public UPrimitiveComponent
{
	GENERATED_UCLASS_BODY()

	void BeginDestroy() override;

	/** Which player index this motion controller should automatically follow */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintSetter = SetAssociatedPlayerIndex, Category = "MotionController")
	int32 PlayerIndex;

	/** Which hand this component should automatically follow */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintSetter = SetTrackingSource, Category = "MotionController")
	EControllerHand Hand;

	/** If false, render transforms within the motion controller hierarchy will be updated a second time immediately before rendering. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MotionController")
	uint32 bDisableLowLatencyUpdate:1;

	/** The tracking status for the device (e.g. full tracking, inertial tracking only, no tracking) */
	UPROPERTY(BlueprintReadOnly, Category = "MotionController")
	ETrackingStatus CurrentTrackingStatus;

	void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

	/** Whether or not this component had a valid tracked device this frame */
	UFUNCTION(BlueprintPure, Category = "MotionController")
	bool IsTracked() const
	{
		return bTracked;
	}

	/** Used to automatically render a model associated with the set hand. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintSetter=SetShowDeviceModel, Category="Visualization")
	bool bDisplayDeviceModel;

	UFUNCTION(BlueprintSetter)
	void SetShowDeviceModel(const bool bShowControllerModel);

	/** Determines the source of the desired model. By default, the active XR system(s) will be queried and (if available) will provide a model for the associated device. NOTE: this may fail if there's no default model; use 'Custom' to specify your own. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintSetter=SetDisplayModelSource, Category="Visualization", meta=(editcondition="bDisplayDeviceModel"))
	FName DisplayModelSource;

	static FName CustomModelSourceId;
	UFUNCTION(BlueprintSetter)
	void SetDisplayModelSource(const FName NewDisplayModelSource);

	/** A mesh override that'll be displayed attached to this MotionController. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintSetter=SetCustomDisplayMesh, Category="Visualization", meta=(editcondition="bDisplayDeviceModel"))
	UStaticMesh* CustomDisplayMesh;

	UFUNCTION(BlueprintSetter)
	void SetCustomDisplayMesh(UStaticMesh* NewDisplayMesh);

	/** Material overrides for the specified display mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visualization", meta=(editcondition="bDisplayDeviceModel"))
	TArray<UMaterialInterface*> DisplayMeshMaterialOverrides;

	UFUNCTION(BlueprintSetter)
	void SetTrackingSource(const EControllerHand NewSource);

	UFUNCTION(BlueprintSetter)
	void SetAssociatedPlayerIndex(const int32 NewPlayer);

public:
	//~ UObject interface
#if WITH_EDITOR
	virtual void PreEditChange(UProperty* PropertyAboutToChange) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif 

public:
	//~ UActorComponent interface
	virtual void OnRegister() override;
	virtual void InitializeComponent() override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

protected:
	//~ Begin UActorComponent Interface.
	virtual void SendRenderTransform_Concurrent() override;
	//~ End UActorComponent Interface.

	void RefreshDisplayComponent(const bool bForceDestroy = false);

private:
	/** Whether or not this component had a valid tracked controller associated with it this frame*/
	bool bTracked;

	/** Whether or not this component has authority within the frame*/
	bool bHasAuthority;

	/** If true, the Position and Orientation args will contain the most recent controller state */
	bool PollControllerState(FVector& Position, FRotator& Orientation, float WorldToMetersScale);

	FTransform RenderThreadRelativeTransform;
	FVector RenderThreadComponentScale;

	/** View extension object that can persist on the render thread without the motion controller component */
	class FViewExtension : public FSceneViewExtensionBase
	{
	public:
		FViewExtension(const FAutoRegister& AutoRegister, UMotionControllerComponent* InMotionControllerComponent);
		virtual ~FViewExtension() {}

		/** ISceneViewExtension interface */
		virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {}
		virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override {}
		virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override;
		virtual void PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView) override {}
		virtual void PreRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& InViewFamily) override;
		virtual void PostRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& InViewFamily) override;
		virtual int32 GetPriority() const override { return -10; }
		virtual bool IsActiveThisFrame(class FViewport* InViewport) const;

	private:
		friend class UMotionControllerComponent;

		/** Motion controller component associated with this view extension */
		UMotionControllerComponent* MotionControllerComponent;
		FLateUpdateManager LateUpdate;
	};
	TSharedPtr< FViewExtension, ESPMode::ThreadSafe > ViewExtension;
 
	UPROPERTY(Transient)
	UPrimitiveComponent* DisplayComponent;

	FXRDeviceId DisplayDeviceId;
};
