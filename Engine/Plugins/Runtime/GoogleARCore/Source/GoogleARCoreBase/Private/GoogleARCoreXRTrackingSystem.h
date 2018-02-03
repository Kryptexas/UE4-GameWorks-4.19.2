// Copyright 2017 Google Inc.

#pragma once
#include "XRTrackingSystemBase.h"
#include "HeadMountedDisplay.h"
#include "IHeadMountedDisplay.h"
#include "SceneViewExtension.h"
#include "SceneViewport.h"
#include "SceneView.h"
#include "GoogleARCoreDevice.h"
#include "ARSystem.h"
#include "ARLightEstimate.h"

class FGoogleARCoreXRTrackingSystem : public FARSystemBase
{
	friend class FGoogleARCoreXRCamera;

public:
	FGoogleARCoreXRTrackingSystem();
	~FGoogleARCoreXRTrackingSystem();

	// FGoogleARCoreXRTrackingSystem Class Method
	void ConfigARCoreXRCamera(bool bMatchCameraFOV, bool bEnablePassthroughRendering);
	void EnableColorCameraRendering(bool bEnableColorCameraRnedering);
	bool GetColorCameraRenderingEnabled();

	// IXRTrackingSystem
	virtual FName GetSystemName() const override;
	virtual bool GetCurrentPose(int32 DeviceId, FQuat& OutOrientation, FVector& OutPosition) override;
	virtual FString GetVersionString() const override;
	virtual bool EnumerateTrackedDevices(TArray<int32>& OutDevices, EXRTrackedDeviceType Type = EXRTrackedDeviceType::Any) override;
	virtual TSharedPtr<class IXRCamera, ESPMode::ThreadSafe> GetXRCamera(int32 DeviceId = HMDDeviceId) override;
	// @todo : can I get rid of this? At least rename to IsCameraTracking / IsTrackingAllowed()
	virtual bool IsHeadTrackingAllowed() const override;
	virtual bool OnStartGameFrame(FWorldContext& WorldContext) override;
	// TODO: Figure out if we need to allow developer set/reset base orientation and position.
	virtual void ResetOrientationAndPosition(float yaw = 0.f) override {}
	// @todo move this to some interface
	virtual float GetWorldToMetersScale() const override;

protected:
	// IARSystemSupport
	virtual void OnARSystemInitialized() override;
	virtual EARTrackingQuality OnGetTrackingQuality() const override;
	virtual void OnStartARSession(UARSessionConfig* SessionConfig) override;
	virtual void OnPauseARSession() override;
	virtual void OnStopARSession() override;
	virtual FARSessionStatus OnGetARSessionStatus() const override;
	virtual void OnSetAlignmentTransform(const FTransform& InAlignmentTransform) override;
	virtual TArray<FARTraceResult> OnLineTraceTrackedObjects(const FVector2D ScreenCoord, EARLineTraceChannels TraceChannels) override;
	virtual TArray<UARTrackedGeometry*> OnGetAllTrackedGeometries() const override;
	virtual TArray<UARPin*> OnGetAllPins() const override;
	virtual bool OnIsTrackingTypeSupported(EARSessionType SessionType) const override;
	virtual UARLightEstimate* OnGetCurrentLightEstimate() const override;

	virtual UARPin* OnPinComponent(USceneComponent* ComponentToPin, const FTransform& PinToWorldTransform, UARTrackedGeometry* TrackedGeometry = nullptr, const FName DebugName = NAME_None) override;
	virtual void OnRemovePin(UARPin* PinToRemove) override;

private:
	//~ FGCObject
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	//~ FGCObject

private:
	FGoogleARCoreDevice* ARCoreDeviceInstance;

	bool bMatchDeviceCameraFOV;
	bool bEnablePassthroughCameraRendering;
	bool bHasValidPose;

	FVector CachedPosition;
	FQuat CachedOrientation;
	FRotator DeltaControlRotation;    // same as DeltaControlOrientation but as rotator
	FQuat DeltaControlOrientation; // same as DeltaControlRotation but as quat

	TSharedPtr<class ISceneViewExtension, ESPMode::ThreadSafe> ViewExtension;

	UARBasicLightEstimate* LightEstimate;
};

DEFINE_LOG_CATEGORY_STATIC(LogGoogleARCoreTrackingSystem, Log, All);
