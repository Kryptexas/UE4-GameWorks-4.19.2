// Copyright 2017 Google Inc.

#pragma once

#include "Components/ActorComponent.h"
#include "GoogleVRPointer.h"
#include "Components/SceneComponent.h"
#include "Classes/GoogleVRControllerFunctionLibrary.h"
#include "Engine/Texture2D.h"
#include "GoogleVRMotionControllerComponent.generated.h"

class UMotionControllerComponent;
class UGoogleVRPointerInputComponent;
class UMaterialInterface;
class UMaterialParameterCollection;
class UGoogleVRLaserPlaneComponent;

/**
 * GoogleVRMotionControllerComponent is a customizable Daydream Motion Controller.
 *
 * It uses the standard unreal MotionControllerComponent to control position and orientation,
 * and adds the following features:
 *
 * Controller Visualization:
 * Renders a skinnable 3D model that responds to button presses on the controller,
 * as well as a laser and reticle.
 *
 * Pointer Input Integration:
 * Integrates with GoogleVRPointerInputComponent so that the motion controller can easily be used to interact with
 * Actors and widgets.
 *
 * Controller Connection Status:
 * The controller visual and pointer input will automatically be turned off when the controller is disconnected.
 * If the component is activated while the controller is disconnected, then the controller visual and pointer input
 * will be off initially. When the controller becomes connected, they will automatically turn on.
 */
UCLASS(ClassGroup=(GoogleVRController), meta=(BlueprintSpawnableComponent))
class GOOGLEVRCONTROLLER_API UGoogleVRMotionControllerComponent : public USceneComponent, public IGoogleVRPointer
{
	GENERATED_BODY()

public:

	UGoogleVRMotionControllerComponent();

	/** Mesh used for controller. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Meshes")
	UStaticMesh* ControllerMesh;

	/** Mesh used for controller touch point. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Meshes")
	UStaticMesh* ControllerTouchPointMesh;

	/** Material used when idle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
	UMaterialInterface* IdleMaterial;

	/** Material used when pressing the touchpad button. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
	UMaterialInterface* TouchpadMaterial;

	/** Material used when pressing the app button. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
	UMaterialInterface* AppMaterial;

	/** Material used when pressing the system button. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
	UMaterialInterface* SystemMaterial;

	/** Material used for touch point when touching the touch pad. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
	UMaterialInterface* ControllerTouchPointMaterial;

	/** Material used for the reticle billboard. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
	UMaterialInterface* ControllerReticleMaterial;

	/** Parameter collection used to set the alpha of all components.
	 *  Must include property named "GoogleVRMotionControllerAlpha".
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
	UMaterialParameterCollection* ParameterCollection;

	/** Mesh used for controller battery state. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery")
	UStaticMesh* ControllerBatteryMesh;

	/** Texture parameter name for the battery material. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery")
	FName BatteryTextureParameterName;

	/** Texture used for the battery unknown state. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery")
	UTexture2D* BatteryUnknownTexture;

	/** Texture used for the battery full state. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery")
	UTexture2D* BatteryFullTexture;

	/** Texture used for the battery almost full state. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery")
	UTexture2D* BatteryAlmostFullTexture;

	/** Texture used for the battery medium state. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery")
	UTexture2D* BatteryMediumTexture;

	/** Texture used for the battery low state. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery")
	UTexture2D* BatteryLowTexture;

	/** Texture used for the battery critcally low state. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery")
	UTexture2D* BatteryCriticalLowTexture;

	/** Texture used for the battery charging state. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery")
	UTexture2D* BatteryChargingTexture;

	/** Static mesh used to represent the laser. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser")
	UStaticMesh* LaserPlaneMesh;

	/** Maximum distance of the pointer (in meters). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser")
	float LaserDistanceMax;

	/** Minimum distance of the reticle (in meters). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reticle")
	float ReticleDistanceMin;

	/** Maximum distance of the reticle (in meters). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reticle")
	float ReticleDistanceMax;

	/** Size of the reticle (in meters) as seen from 1 meter */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reticle")
	float ReticleSize;

	/** The enter radius for the ray is the sprite size multiplied by this value.
	 *  See IGoogleVRPointer.h for details.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ray")
	float EnterRadiusCoeff;

	/** The exit radius for the ray is the sprite size multiplied by this value.
	 *  See IGoogleVRPointer.h for details.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ray")
	float ExitRadiusCoeff;

	/** If true, then a GoogleVRInputComponent will automatically be created if one doesn't already exist. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	bool RequireInputComponent;

	/** TranslucentSortPriority to use when rendering.
	 *  The reticle, the laser, and the controller mesh use TranslucentSortPriority.
	 *  The controller touch point mesh uses TranslucentSortPriority+1, this makes sure that
	 *  the touch point doesn't z-fight with the controller mesh.
	 **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
	int TranslucentSortPriority;

	/** Get the MotionControllerComponent.
	 *  This is the MotionControllerComponent being used to position the
	 *  Controller visuals.
	 *  Can be used if you desire to attach any additional components
	 *  As part of your visualization of the controller.
	 *  @return motion controller component
	 */
	UFUNCTION(BlueprintCallable, Category = "GoogleVRMotionController", meta = (Keywords = "Cardboard AVR GVR"))
	UMotionControllerComponent* GetMotionController() const;

	/** Get the StaticMeshComponent used to represent the controller.
	 *  Can be used if you desire to modify the controller at runtime
	 *  @return controller static mesh component.
	 */
	UFUNCTION(BlueprintCallable, Category = "GoogleVRMotionController", meta = (Keywords = "Cardboard AVR GVR"))
	UStaticMeshComponent* GetControllerMesh() const;

	/** Get the StaticMeshComponent used to represent the laser.
	*  Can be used if you desire to modify the laser at runtime
	*  @return laser static mesh component.
	*/
	UFUNCTION(BlueprintCallable, Category = "GoogleVRMotionController", meta = (Keywords = "Cardboard AVR GVR"))
	UStaticMeshComponent* GetLaser() const;

	/** Get the MaterialInstanceDynamic used to represent the laser material.
	*  Can be used if you desire to modify the laser at runtime
	*  (i.e. change laser color when pointing at object).
	*  @return laser dynamic material instance.
	*/
	UFUNCTION(BlueprintCallable, Category = "GoogleVRMotionController", meta = (Keywords = "Cardboard AVR GVR"))
	UMaterialInstanceDynamic* GetLaserMaterial() const;

	/** Get the MaterialBillboardComponent used to represent the reticle.
	 *  Can be used if you desire to modify the reticle at runtime
	 *  @return reticle billboard component.
	 */
	UFUNCTION(BlueprintCallable, Category = "GoogleVRMotionController", meta = (Keywords = "Cardboard AVR GVR"))
	UMaterialBillboardComponent* GetReticle() const;

	/** Set the distance of the pointer.
	 *  This will update the distance of the laser and the reticle
	 *  based upon the min and max distances.
	 *  @param Distance - new distance
	 */
	UFUNCTION(BlueprintCallable, Category = "GoogleVRMotionController", meta = (Keywords = "Cardboard AVR GVR"))
	void SetPointerDistance(float Distance);

	/** IGoogleVRPointer Implementation */
	virtual void OnPointerEnter(const FHitResult& HitResult, bool IsHitInteractive) override;
	virtual void OnPointerHover(const FHitResult& HitResult, bool IsHitInteractive) override;
	virtual void OnPointerExit(const FHitResult& HitResult) override;
	virtual FVector GetOrigin() const override;
	virtual FVector GetDirection() const override;
	virtual void GetRadius(float& OutEnterRadius, float& OutExitRadius) const override;
	virtual float GetMaxPointerDistance() const override;
	virtual bool IsPointerActive() const override;

	/** ActorComponent Overrides */
	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction ) override;

	/** If the controller is connected, this will enable the controller visual and pointer input.
	 *  Otherwise, it will start polling for when the controller connects. When it does, the controller visual
	 *  and pointer input will be enabled.
	 */
	virtual void Activate(bool bReset=false) override;

	/** Disables the controller visual and pointer input. */
	virtual void Deactivate() override;

private:

	void TrySetControllerMaterial(UMaterialInterface* NewMaterial);
	void UpdateBatteryIndicator();
	void UpdateLaserDistance(float Distance);
	void UpdateLaserCorrection(FVector Correction);
	void UpdateReticleDistance(float Distance);
	void UpdateReticleLocation(FVector Location, FVector OriginLocation);
	void UpdateReticleSize();
	void SetSubComponentsEnabled(bool bNewEnabled);
	bool IsControllerConnected() const;
	float GetWorldToMetersScale() const;

	APlayerController* PlayerController;

	UMotionControllerComponent* MotionControllerComponent;
	UStaticMeshComponent* ControllerMeshComponent;
	UStaticMeshComponent* ControllerTouchPointMeshComponent;
	UStaticMeshComponent* ControllerBatteryMeshComponent;
	UMaterialInterface* ControllerBatteryStaticMaterial;
	UMaterialInstanceDynamic* ControllerBatteryMaterial;
	USceneComponent* PointerContainerComponent;
	UGoogleVRLaserPlaneComponent* LaserPlaneComponent;
	UMaterialBillboardComponent* ReticleBillboardComponent;

	FVector TouchMeshScale;
	bool bAreSubComponentsEnabled;
	EGoogleVRControllerBatteryLevel LastKnownBatteryState;
	bool bBatteryWasCharging;

	static constexpr float CONTROLLER_OFFSET_RATIO = 0.8f;
	static constexpr float TOUCHPAD_RADIUS = 0.015f;
	static constexpr float TOUCHPAD_POINT_X_OFFSET = 0.041f;
	static constexpr float TOUCHPAD_POINT_ELEVATION = 0.0025f;
	static constexpr float TOUCHPAD_POINT_FILTER_STRENGTH = 0.8f;
	static const FVector TOUCHPAD_POINT_DIMENSIONS;
	static const FVector BATTERY_INDICATOR_TRANSLATION;
	static const FVector BATTERY_INDICATOR_SCALE;
	static const FQuat BATTERY_INDICATOR_ROTATION;
};
