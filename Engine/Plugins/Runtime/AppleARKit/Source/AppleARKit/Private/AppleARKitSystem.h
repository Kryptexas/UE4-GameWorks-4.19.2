// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "XRTrackingSystemBase.h"
#include "AppleARKitConfiguration.h"

// ARKit
#if ARKIT_SUPPORT
#import <ARKit/ARKit.h>
#include "AppleARKitSessionDelegate.h"
#endif // ARKIT_SUPPORT

//
//  FAppleARKitSystem
//

struct FAppleARKitFrame;

class FAppleARKitSystem : public FXRTrackingSystemBase, public TSharedFromThis<FAppleARKitSystem, ESPMode::ThreadSafe>
{
	friend class FAppleARKitXRCamera;
	
public:
	FAppleARKitSystem();
	
private:
	//~ IXRTrackingSystem
	FName GetSystemName() const override;
	bool GetCurrentPose(int32 DeviceId, FQuat& OutOrientation, FVector& OutPosition) override;
	FString GetVersionString() const override;
	bool EnumerateTrackedDevices(TArray<int32>& OutDevices, EXRTrackedDeviceType Type) override;
	void RefreshPoses() override;
	void ResetOrientationAndPosition(float Yaw) override;
	bool IsHeadTrackingAllowed() const override;
	TSharedPtr<class IXRCamera, ESPMode::ThreadSafe> GetXRCamera(int32 DeviceId) override;
	float GetWorldToMetersScale() const override;
	//~ IXRTrackingSystem
	
	void Run();
	bool RunWithConfiguration(const FAppleARKitConfiguration& InConfiguration);
	bool IsRunning() const;
	bool Pause();
	
public:
	// Session delegate callbacks
	void SessionDidUpdateFrame_DelegateThread( TSharedPtr< FAppleARKitFrame, ESPMode::ThreadSafe > Frame );
	void SessionDidFailWithError_DelegateThread( const FString& Error );
#if ARKIT_SUPPORT
	void SessionDidAddAnchors_DelegateThread( NSArray<ARAnchor*>* anchors );
	void SessionDidUpdateAnchors_DelegateThread( NSArray<ARAnchor*>* anchors );
	void SessionDidRemoveAnchors_DelegateThread( NSArray<ARAnchor*>* anchors );
#endif // ARKIT_SUPPORT
	
private:
	
	bool bIsRunning = false;
	
#if ARKIT_SUPPORT
	
	// ARKit Session
	ARSession* Session = nullptr;
	
	// ARKit Session Delegate
	FAppleARKitSessionDelegate* Delegate = nullptr;
	
	/** The Metal texture cache for unbuffered texture uploads. */
	CVMetalTextureCacheRef MetalTextureCache = nullptr;
	
#endif // ARKIT_SUPPORT
	
	
	// The frame number when LastReceivedFrame was last updated
	uint32 GameThreadFrameNumber;

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
	APPLEARKIT_API TSharedPtr<class IXRTrackingSystem, ESPMode::ThreadSafe> CreateAppleARKitSystem();
}

