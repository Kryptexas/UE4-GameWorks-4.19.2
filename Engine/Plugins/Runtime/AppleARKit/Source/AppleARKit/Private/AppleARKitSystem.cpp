// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "AppleARKitSystem.h"
#include "DefaultXRCamera.h"
#include "AppleARKitSessionDelegate.h"
#include "ScopeLock.h"
#include "AppleARKitPrivate.h"
#include "AppleARKitTransform.h"
#include "AppleARKitVideoOverlay.h"
#include "AppleARKitFrame.h"

//
//  FAppleARKitXRCamera
//

class FAppleARKitXRCamera : public FDefaultXRCamera
{
public:
	FAppleARKitXRCamera(const FAutoRegister& AutoRegister, FAppleARKitSystem& InTrackingSystem, int32 InDeviceId)
	: FDefaultXRCamera( AutoRegister, &InTrackingSystem, InDeviceId )
	, ARKitSystem( InTrackingSystem )
	{}
	
private:
	//~ FDefaultXRCamera
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override
	{
	}
	
	virtual void SetupViewProjectionMatrix(FSceneViewProjectionData& InOutProjectionData) override
	{
	}
	
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override
	{
	}
	
	virtual void PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView) override
	{
	}
	
	virtual void PreRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& InViewFamily) override
	{
		// Grab the latest frame from ARKit
		{
			FScopeLock ScopeLock(&ARKitSystem.FrameLock);
			ARKitSystem.RenderThreadFrame = ARKitSystem.LastReceivedFrame;
		}
		
		// @todo arkit: Camera late update
		
		if (ARKitSystem.RenderThreadFrame.IsValid())
		{
			VideoOverlay.UpdateVideoTexture_RenderThread(RHICmdList, *ARKitSystem.RenderThreadFrame);
		}
	}
	
	virtual void PostRenderMobileBasePass_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView) override
	{
		VideoOverlay.RenderVideoOverlay_RenderThread(RHICmdList, InView);
	}
	
	virtual bool IsActiveThisFrame(class FViewport* InViewport) const override
	{
		return true;
	}
	//~ FDefaultXRCamera
	
private:
	FAppleARKitSystem& ARKitSystem;
	FAppleARKitVideoOverlay VideoOverlay;
};




//
//  FAppleARKitSystem
//

FAppleARKitSystem::FAppleARKitSystem()
{
	Run();
}
	
FName FAppleARKitSystem::GetSystemName() const
{
	static const FName AppleARKitSystemName(TEXT("AppleARKit"));
	return AppleARKitSystemName;
}


bool FAppleARKitSystem::GetCurrentPose(int32 DeviceId, FQuat& OutOrientation, FVector& OutPosition)
{
	if (DeviceId == IXRTrackingSystem::HMDDeviceId && GameThreadFrame.IsValid())
	{
		// Do not have to lock here, because we are on the game
		// thread and GameThreadFrame is only written to from the game thread.
		OutOrientation = GameThreadFrame->Camera.Orientation;
		OutPosition = GameThreadFrame->Camera.Translation;
		
		return true;
	}
	else
	{
		return false;
	}
}

FString FAppleARKitSystem::GetVersionString() const
{
	return TEXT("AppleARKit - V1.0");
}


bool FAppleARKitSystem::EnumerateTrackedDevices(TArray<int32>& OutDevices, EXRTrackedDeviceType Type)
{
	if (Type == EXRTrackedDeviceType::Any || Type == EXRTrackedDeviceType::HeadMountedDisplay)
	{
		static const int32 DeviceId = IXRTrackingSystem::HMDDeviceId;
		OutDevices.Add(DeviceId);
		return true;
	}
	return false;
}


void FAppleARKitSystem::RefreshPoses()
{
	{
		FScopeLock ScopeLock( &FrameLock );
		GameThreadFrame = LastReceivedFrame;
	}
}


void FAppleARKitSystem::ResetOrientationAndPosition(float Yaw)
{
}

bool FAppleARKitSystem::IsHeadTrackingAllowed() const
{
#if ARKIT_SUPPORT
	return true;
#else
	return false;
#endif
}

TSharedPtr<class IXRCamera, ESPMode::ThreadSafe> FAppleARKitSystem::GetXRCamera(int32 DeviceId)
{
	if (!XRCamera.IsValid())
	{
		TSharedRef<FAppleARKitXRCamera, ESPMode::ThreadSafe> NewCamera = FSceneViewExtensions::NewExtension<FAppleARKitXRCamera>(*this, DeviceId);
		XRCamera = NewCamera;
	}
	
	return XRCamera;
}

float FAppleARKitSystem::GetWorldToMetersScale() const
{
	// @todo arkit FAppleARKitSystem::GetWorldToMetersScale needs a real scale somehow
	return 100.0f;
}

void FAppleARKitSystem::Run()
{
	// @todo arkit FAppleARKitSystem::GetWorldToMetersScale needs a real scale somehow
	FAppleARKitConfiguration Config;
	RunWithConfiguration( Config );
}

bool FAppleARKitSystem::RunWithConfiguration( const FAppleARKitConfiguration& InConfiguration )
{

	if (IsRunning())
	{
		UE_LOG(LogAppleARKit, Log, TEXT("Session already running"), this);
		
		return true;
	}

#if ARKIT_SUPPORT
	
	ARSessionRunOptions options = 0;
	
	// Create our ARSessionDelegate
	if (Delegate == nullptr)
	{
		Delegate =[[FAppleARKitSessionDelegate alloc] initWithAppleARKitSystem:this];
	}
	
	if (Session == nullptr)
	{
		// Start a new ARSession
		Session = [ARSession new];
		Session.delegate = Delegate;
		Session.delegateQueue = dispatch_get_global_queue(QOS_CLASS_USER_INTERACTIVE, 0);
	}
	else
	{
		// pause and start with new options
		options = ARSessionRunOptionResetTracking | ARSessionRunOptionRemoveExistingAnchors;
		[Session pause];
	}
	
	// Create MetalTextureCache
	if (IsMetalPlatform(GMaxRHIShaderPlatform))
	{
		id<MTLDevice> Device = (id<MTLDevice>)GDynamicRHI->RHIGetNativeDevice();
		check(Device);
		
		CVReturn Return = CVMetalTextureCacheCreate(nullptr, nullptr, Device, nullptr, &MetalTextureCache);
		check(Return == kCVReturnSuccess);
		check(MetalTextureCache);
		
		// Pass to session delegate to use for Metal texture creation
		[Delegate setMetalTextureCache:MetalTextureCache];
	}
	
	// Convert to native ARWorldTrackingSessionConfiguration
	ARConfiguration* Configuration = FAppleARKitConfiguration::ToARConfiguration(InConfiguration);
	
	UE_LOG(LogAppleARKit, Log, TEXT("Starting session: %p with options %d"), this, options);
	
	// Start the session with the configuration
	[Session runWithConfiguration:Configuration options:options];
	
#endif // ARKIT_SUPPORT
	
	// @todo arkit BaseTransform = FTransform::Identity;
	
	// Set running state
	bIsRunning = true;
	
	return true;
}

bool FAppleARKitSystem::IsRunning() const
{
	return bIsRunning;
}

bool FAppleARKitSystem::Pause()
{
	// Already stopped?
	if (!IsRunning())
	{
		return true;
	}
	
	UE_LOG(LogAppleARKit, Log, TEXT("Stopping session: %p"), this);
	
#if ARKIT_SUPPORT
	
	// Suspend the session
	[Session pause];
	
	// Release MetalTextureCache created in Start
	if (MetalTextureCache)
	{
		// Tell delegate to release it
		[Delegate setMetalTextureCache:nullptr];
		
		CFRelease(MetalTextureCache);
		MetalTextureCache = nullptr;
	}
	
#endif // ARKIT_SUPPORT
	
	// Set running state
	bIsRunning = false;
	
	return true;
}
						
						
void FAppleARKitSystem::SessionDidUpdateFrame_DelegateThread(TSharedPtr< FAppleARKitFrame, ESPMode::ThreadSafe > Frame)
{
	// Thread safe swap buffered frame
	FScopeLock ScopeLock(&FrameLock);
	LastReceivedFrame = Frame;
}
			
void FAppleARKitSystem::SessionDidFailWithError_DelegateThread(const FString& Error)
{
	UE_LOG(LogAppleARKit, Warning, TEXT("Session failed with error: %s"), *Error);
}

#if ARKIT_SUPPORT

void FAppleARKitSystem::SessionDidAddAnchors_DelegateThread( NSArray<ARAnchor*>* anchors )
{
//  @todo arkit
//	FScopeLock ScopeLock( &AnchorsLock );
//
//	for (ARAnchor* anchor in anchors)
//	{
//		// Construct appropriate UAppleARKitAnchor subclass
//		UAppleARKitAnchor* Anchor;
//		if ([anchor isKindOfClass:[ARPlaneAnchor class]])
//		{
//			Anchor = NewObject< UAppleARKitPlaneAnchor >();
//		}
//		else
//		{
//			Anchor = NewObject< UAppleARKitAnchor >();
//		}
//
//		// Set UUID
//		ToFGuid( anchor.identifier, Anchor->Identifier );
//
//		// Update fields
//		Anchor->Update_DelegateThread( anchor );
//
//		// Map to UUID
//		Anchors.Add( Anchor->Identifier, Anchor );
//	}
}

void FAppleARKitSystem::SessionDidUpdateAnchors_DelegateThread( NSArray<ARAnchor*>* anchors )
{
//  @todo arkit
//	FScopeLock ScopeLock( &AnchorsLock );
//
//	for (ARAnchor* anchor in anchors)
//	{
//		// Convert to FGuid
//		FGuid Identifier;
//		ToFGuid( anchor.identifier, Identifier );
//
//
//		// Lookup in map
//		if ( UAppleARKitAnchor** Anchor = Anchors.Find( Identifier ) )
//		{
//			// Update fields
//			(*Anchor)->Update_DelegateThread( anchor );
//		}
//	}
}

void FAppleARKitSystem::SessionDidRemoveAnchors_DelegateThread( NSArray<ARAnchor*>* anchors )
{
//  @todo arkit
//	FScopeLock ScopeLock( &AnchorsLock );
//
//	for (ARAnchor* anchor in anchors)
//	{
//		// Convert to FGuid
//		FGuid Identifier;
//		ToFGuid( anchor.identifier, Identifier );
//
//		// Remove from map (allowing anchor to be garbage collected)
//		Anchors.Remove( Identifier );
//	}
}

#endif //ARKIT_SUPPORT






namespace AppleARKitSupport
{
	TSharedPtr<class IXRTrackingSystem, ESPMode::ThreadSafe> CreateAppleARKitSystem()
	{
		return TSharedPtr<class FAppleARKitSystem, ESPMode::ThreadSafe>(new FAppleARKitSystem());
	}
}
