// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/SceneCaptureComponent2D.h"
#include "InputCoreTypes.h" // for EControllerHand
#include "Math/Color.h" // for FLinearColor
#include "MixedRealityCaptureComponent.generated.h"

class UMediaPlayer;
class UMaterial;
class APlayerController;
class UMixedRealityBillboard;

/**
 *	
 */
UCLASS(ClassGroup = Rendering, editinlinenew, meta = (BlueprintSpawnableComponent))
class MIXEDREALITYFRAMEWORK_API UMixedRealityCaptureComponent : public USceneCaptureComponent2D
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=VideoCapture)
	UMediaPlayer* MediaSource;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=VideoCapture)
	UMaterial* VideoProcessingMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=VideoCapture)
	FLinearColor ChromaColor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Tracking)
	bool bAutoTracking;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Tracking, meta=(editcondition="bAutoTracking"))
	EControllerHand TrackingDevice;

public:
	// UObject interface
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	// End of UObject interface

	// UActorComponent interface
	virtual void OnRegister() override;
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;
	virtual void OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport) override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
#if WITH_EDITOR
	virtual void PreEditChange(UProperty* PropertyThatWillChange) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif 
	// End of UActorComponent interface

	// USceneComponent interface
#if WITH_EDITOR
	virtual bool GetEditorPreviewInfo(float DeltaTime, FMinimalViewInfo& ViewOut) override;
#endif 
	// End of USceneComponent interface

public:
	//~ USceneCaptureComponent interface

	virtual const AActor* GetViewOwner() const override { return GetOwner(); }

private:
	/**
	 *
	 */
	float GetDesiredAspectRatio() const;

	/**
	 *
	 */
	void AttachMediaListeners() const;

	/**
	 *
	 */
	void DetachMediaListeners() const;

	/**
	 *
	 */
	UFUNCTION()
	void OnVideoFeedOpened(FString MediaUrl);

	/**
	 *
	 */
	void RefreshProjectionDimensions();


	APlayerController* FindTargetPlayer() const;
	

#if WITH_EDITORONLY_DATA
	UPROPERTY(transient)
	UStaticMesh* ProxyMesh;

	UStaticMeshComponent* ProxyMeshComponent;
#endif
	
	UPROPERTY(EditAnywhere, Category=Tracking)
	TEnumAsByte<EAutoReceiveInput::Type> AutoAttach;

	UPROPERTY(transient)
	UMixedRealityBillboard* VideoProjectionComponent;
};
