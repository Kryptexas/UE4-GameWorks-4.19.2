// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "XRTrackingSystemBase.h"
#include "AppleARKitConfiguration.h"
#include "ARSystem.h"
#include "AppleARKitHitTestResult.h"
#include "AppleARKitLiveLinkSourceFactory.h"
#include "Kismet/BlueprintPlatformLibrary.h"

// ARKit
#if ARKIT_SUPPORT && __IPHONE_OS_VERSION_MAX_ALLOWED >= 110000
#import <ARKit/ARKit.h>
#include "AppleARKitSessionDelegate.h"
#endif // ARKIT_SUPPORT


DECLARE_STATS_GROUP(TEXT("AppleARKit"), STATGROUP_APPLEARKIT, STATCAT_Advanced);

//
//  FAppleARKitSystem
//

struct FAppleARKitFrame;
struct FAppleARKitAnchorData;

class FAppleARKitSystem : public FARSystemBase
{
	friend class FAppleARKitXRCamera;
	
	
public:
	FAppleARKitSystem();
	~FAppleARKitSystem();
	
	//~ IXRTrackingSystem
	FName GetSystemName() const override;
	bool GetCurrentPose(int32 DeviceId, FQuat& OutOrientation, FVector& OutPosition) override;
	FString GetVersionString() const override;
	bool EnumerateTrackedDevices(TArray<int32>& OutDevices, EXRTrackedDeviceType Type) override;
	void ResetOrientationAndPosition(float Yaw) override;
	bool IsHeadTrackingAllowed() const override;
	TSharedPtr<class IXRCamera, ESPMode::ThreadSafe> GetXRCamera(int32 DeviceId) override;
	float GetWorldToMetersScale() const override;
	void OnBeginRendering_GameThread() override;
	bool OnStartGameFrame(FWorldContext& WorldContext) override;
	//~ IXRTrackingSystem
	
	// @todo arkit : this is for the blueprint library only; try to get rid of this method
	bool GetCurrentFrame(FAppleARKitFrame& OutCurrentFrame) const;
private:
	//~ FGCObject
	virtual void AddReferencedObjects( FReferenceCollector& Collector ) override;
	//~ FGCObject
protected:
	//~IARSystemSupport
	virtual void OnARSystemInitialized() override;
	virtual EARTrackingQuality OnGetTrackingQuality() const override;
	virtual void OnStartARSession(UARSessionConfig* SessionConfig) override;
	virtual void OnPauseARSession() override;
	virtual void OnStopARSession() override;
	virtual FARSessionStatus OnGetARSessionStatus() const override;
	virtual void OnSetAlignmentTransform(const FTransform& InAlignmentTransform) override;
	virtual TArray<FARTraceResult> OnLineTraceTrackedObjects( const FVector2D ScreenCoord, EARLineTraceChannels TraceChannels ) override;
	virtual TArray<UARTrackedGeometry*> OnGetAllTrackedGeometries() const override;
	virtual TArray<UARPin*> OnGetAllPins() const override;
	virtual bool OnIsTrackingTypeSupported(EARSessionType SessionType) const override;
	virtual UARLightEstimate* OnGetCurrentLightEstimate() const override;
	virtual UARPin* OnPinComponent(USceneComponent* ComponentToPin, const FTransform& PinToWorldTransform, UARTrackedGeometry* TrackedGeometry = nullptr, const FName DebugName = NAME_None) override;
	virtual void OnRemovePin(UARPin* PinToRemove) override;
	//~IARSystemSupport

private:
	bool Run(UARSessionConfig* SessionConfig);
	bool IsRunning() const;
	bool Pause();
	void OrientationChanged(const int32 NewOrientation);
	void UpdatePoses();
	void UpdateFrame();

public:
	// Session delegate callbacks
	void SessionDidUpdateFrame_DelegateThread( TSharedPtr< FAppleARKitFrame, ESPMode::ThreadSafe > Frame );
	void SessionDidFailWithError_DelegateThread( const FString& Error );
#if ARKIT_SUPPORT && __IPHONE_OS_VERSION_MAX_ALLOWED >= 110000
	void SessionDidAddAnchors_DelegateThread( NSArray<ARAnchor*>* anchors );
	void SessionDidUpdateAnchors_DelegateThread( NSArray<ARAnchor*>* anchors );
	void SessionDidRemoveAnchors_DelegateThread( NSArray<ARAnchor*>* anchors );
private:
	void SessionDidAddAnchors_Internal( TSharedRef<FAppleARKitAnchorData> AnchorData );
	void SessionDidUpdateAnchors_Internal( TSharedRef<FAppleARKitAnchorData> AnchorData );
	void SessionDidRemoveAnchors_Internal( FGuid AnchorGuid );
#endif // ARKIT_SUPPORT
	void SessionDidUpdateFrame_Internal( TSharedRef< FAppleARKitFrame, ESPMode::ThreadSafe > Frame );

	
public:
	/**
	 * Searches the last processed frame for anchors corresponding to a point in the captured image.
	 *
	 * A 2D point in the captured image's coordinate space can refer to any point along a line segment
	 * in the 3D coordinate space. Hit-testing is the process of finding anchors of a frame located along this line segment.
	 *
	 * NOTE: The hit test locations are reported in ARKit space. For hit test results
	 * in game world coordinates, you're after UAppleARKitCameraComponent::HitTestAtScreenPosition
	 *
	 * @param ScreenPosition The viewport pixel coordinate of the trace origin.
	 */
	UFUNCTION( BlueprintCallable, Category="AppleARKit|Session" )
	bool HitTestAtScreenPosition( const FVector2D ScreenPosition, EAppleARKitHitTestResultType Types, TArray< FAppleARKitHitTestResult >& OutResults );
	
	
private:
	
	bool bIsRunning = false;
	
	void SetDeviceOrientation( EScreenOrientation::Type InOrientation );
	
	/** The orientation of the device; see EScreenOrientation */
	EScreenOrientation::Type DeviceOrientation;
	
	/** A rotation from ARKit TrackingSpace to Unreal Space. It is re-derived based on other parameters; users should not set it directly. */
	FRotator DerivedTrackingToUnrealRotation;
	
#if ARKIT_SUPPORT && __IPHONE_OS_VERSION_MAX_ALLOWED >= 110000

	// ARKit Session
	ARSession* Session = nullptr;
	
	// ARKit Session Delegate
	FAppleARKitSessionDelegate* Delegate = nullptr;
	
	/** The Metal texture cache for unbuffered texture uploads. */
	CVMetalTextureCacheRef MetalTextureCache = nullptr;
	
#endif // ARKIT_SUPPORT

	//
	// PROPERTIES REPORTED TO FGCObject
	// ...
	TMap< FGuid, UARTrackedGeometry* > TrackedGeometries;
	TArray<UARPin*> Pins;
	UARLightEstimate* LightEstimate;
	// ...
	// PROPERTIES REPORTED TO FGCObject
	//
	

	/** The ar frame number when LastReceivedFrame was last updated */
	uint32 GameThreadFrameNumber;
	/** The ar timestamp of when the LastReceivedFrame was last updated */
	double GameThreadTimestamp;

	/** If requested, publishes face ar updates to LiveLink for the animation system to use */
	TSharedPtr<ILiveLinkSourceARKit> LiveLinkSource;
	/** Copied from the UARSessionConfig project settings object */
	FName FaceTrackingLiveLinkSubjectName;
	
	
	// An int counter that provides a human-readable debug number for Tracked Geometries.
	uint32 LastTrackedGeometry_DebugId;

	//'threadsafe' sharedptrs merely guaranteee atomicity when adding/removing refs.  You can still have a race
	//with destruction and copying sharedptrs.
	FCriticalSection FrameLock;

	// Last frame grabbed & processed by via the ARKit session delegate
	TSharedPtr< FAppleARKitFrame, ESPMode::ThreadSafe > GameThreadFrame;
	TSharedPtr< FAppleARKitFrame, ESPMode::ThreadSafe > RenderThreadFrame;
	TSharedPtr< FAppleARKitFrame, ESPMode::ThreadSafe > LastReceivedFrame;
};


namespace AppleARKitSupport
{
	APPLEARKIT_API TSharedPtr<class FAppleARKitSystem, ESPMode::ThreadSafe> CreateAppleARKitSystem();
}

