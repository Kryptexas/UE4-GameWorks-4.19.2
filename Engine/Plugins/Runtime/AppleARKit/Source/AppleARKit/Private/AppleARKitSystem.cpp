// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "AppleARKitSystem.h"
#include "DefaultXRCamera.h"
#include "AppleARKitSessionDelegate.h"
#include "ScopeLock.h"
#include "AppleARKitModule.h"
#include "AppleARKitTransform.h"
#include "AppleARKitVideoOverlay.h"
#include "AppleARKitFrame.h"
#if ARKIT_SUPPORT && __IPHONE_OS_VERSION_MAX_ALLOWED >= 110000
#include "IOSAppDelegate.h"
#endif
#include "AppleARKitAnchor.h"
#include "AppleARKitPlaneAnchor.h"
#include "GeneralProjectSettings.h"
#include "IOSRuntimeSettings.h"

// For orientation changed
#include "Misc/CoreDelegates.h"

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
	void OverrideFOV(float& InOutFOV)
	{
		// @todo arkit : is it safe not to lock here? Theoretically this should only be called on the game thread.
		ensure(IsInGameThread());
		if (ARKitSystem.GameThreadFrame.IsValid())
		{
			if (ARKitSystem.DeviceOrientation == EScreenOrientation::Portrait || ARKitSystem.DeviceOrientation == EScreenOrientation::PortraitUpsideDown)
			{
				// Portrait
				InOutFOV = ARKitSystem.GameThreadFrame->Camera.GetVerticalFieldOfViewForScreen(EAppleARKitBackgroundFitMode::Fill);
			}
			else
			{
				// Landscape
				InOutFOV = ARKitSystem.GameThreadFrame->Camera.GetHorizontalFieldOfViewForScreen(EAppleARKitBackgroundFitMode::Fill);
			}
		}
	}
	
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override
	{
		FDefaultXRCamera::SetupView(InViewFamily, InView);
	}
	
	virtual void SetupViewProjectionMatrix(FSceneViewProjectionData& InOutProjectionData) override
	{
		FDefaultXRCamera::SetupViewProjectionMatrix(InOutProjectionData);
	}
	
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override
	{
		FDefaultXRCamera::BeginRenderViewFamily(InViewFamily);
	}
	
	virtual void PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView) override
	{
		FDefaultXRCamera::PreRenderView_RenderThread(RHICmdList, InView);
	}
	
	virtual void PreRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& InViewFamily) override
	{
		// Grab the latest frame from ARKit
		{
			FScopeLock ScopeLock(&ARKitSystem.FrameLock);
			ARKitSystem.RenderThreadFrame = ARKitSystem.LastReceivedFrame;
		}
		
		// @todo arkit: Camera late update? 
		
		if (ARKitSystem.RenderThreadFrame.IsValid())
		{
			VideoOverlay.UpdateVideoTexture_RenderThread(RHICmdList, *ARKitSystem.RenderThreadFrame);
		}
		
		FDefaultXRCamera::PreRenderViewFamily_RenderThread(RHICmdList, InViewFamily);
	}
	
	virtual void PostRenderMobileBasePass_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView) override
	{
		VideoOverlay.RenderVideoOverlay_RenderThread(RHICmdList, InView, ARKitSystem.DeviceOrientation);
		
		FDefaultXRCamera::PostRenderMobileBasePass_RenderThread(RHICmdList, InView);
	}
	
	virtual bool IsActiveThisFrame(class FViewport* InViewport) const override
	{
		// Base implementation needs this call as it updates bCurrentFrameIsStereoRendering as a side effect.
		// We'll ignore the result however.
		FDefaultXRCamera::IsActiveThisFrame(InViewport);

#if ARKIT_SUPPORT && __IPHONE_OS_VERSION_MAX_ALLOWED >= 110000
		if ([IOSAppDelegate GetDelegate].OSVersion >= 11.0f)
		{
			return GetDefault<UGeneralProjectSettings>()->bStartInAR;
		}
		else
		{
			return false;
		}
#else
		return false;
#endif //ARKIT_SUPPORT
	}
	//~ FDefaultXRCamera
	
private:
	FAppleARKitSystem& ARKitSystem;
	FAppleARKitVideoOverlay VideoOverlay;
};



#if ARKIT_SUPPORT && __IPHONE_OS_VERSION_MAX_ALLOWED >= 110000
static FORCEINLINE FGuid ToFGuid( uuid_t UUID )
{
	FGuid AsGUID(
		*(uint32*)UUID,
		*((uint32*)UUID)+1,
		*((uint32*)UUID)+2,
		*((uint32*)UUID)+3);
	return AsGUID;
}

static FORCEINLINE FGuid ToFGuid( NSUUID* Identifier )
{
	// Get bytes
	uuid_t UUID;
	[Identifier getUUIDBytes:UUID];
	
	// Set FGuid parts
	return ToFGuid( UUID );
}

enum class EAppleAnchorType : uint8
{
	Anchor,
	PlaneAnchor,
	MAX
};

struct FAppleARKitAnchorData
{
	FAppleARKitAnchorData(FGuid InAnchorGuid, FTransform InTransform, FVector InCenter, FVector InExtent)
	: Transform( InTransform )
	, Center(InCenter)
	, Extent(InExtent)
	, AnchorType( EAppleAnchorType::PlaneAnchor )
	, AnchorGUID(InAnchorGuid )
	{
	}
	
	FAppleARKitAnchorData(FGuid InAnchorGuid, FTransform InTransform)
	: Transform( InTransform )
	, Center()
	, Extent()
	, AnchorType( EAppleAnchorType::Anchor )
	, AnchorGUID( InAnchorGuid )
	{
	}
	
	FTransform Transform;
	FVector Center;
	FVector Extent;
	EAppleAnchorType AnchorType;
	FGuid AnchorGUID;
};

#endif//ARKIT_SUPPORT && __IPHONE_OS_VERSION_MAX_ALLOWED >= 110000






//
//  FAppleARKitSystem
//

FAppleARKitSystem::FAppleARKitSystem()
: DeviceOrientation(EScreenOrientation::Unknown)
, DerivedTrackingToUnrealRotation(FRotator::ZeroRotator)
{
	// See Initialize(), as we need access to SharedThis()
}

FAppleARKitSystem::~FAppleARKitSystem()
{
	// Unregister our ability to hit-test in AR with Unreal
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
		OutOrientation = GameThreadFrame->Camera.Orientation * DerivedTrackingToUnrealRotation.Quaternion();
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

FRotator DeriveTrackingToWorldRotation( EScreenOrientation::Type DeviceOrientation )
{
	// We rotate the camera to counteract the portrait vs. landscape viewport rotation
	FRotator DeviceRot = FRotator::ZeroRotator;
	switch (DeviceOrientation)
	{
		case EScreenOrientation::Portrait:
			DeviceRot = FRotator(0.0f, 0.0f, -90.0f);
			break;
			
		case EScreenOrientation::PortraitUpsideDown:
			DeviceRot = FRotator(0.0f, 0.0f, 90.0f);
			break;
			
		default:
		case EScreenOrientation::LandscapeLeft:
			break;
			
		case EScreenOrientation::LandscapeRight:
			DeviceRot = FRotator(0.0f, 0.0f, 180.0f);
			break;
	};
	
	return DeviceRot;
}

void FAppleARKitSystem::UpdatePoses()
{
	if (DeviceOrientation == EScreenOrientation::Unknown)
	{
		SetDeviceOrientation( static_cast<EScreenOrientation::Type>(FPlatformMisc::GetDeviceOrientation()) );
	}
	
	{
		FScopeLock ScopeLock( &FrameLock );
		GameThreadFrame = LastReceivedFrame;
	}
}


void FAppleARKitSystem::ResetOrientationAndPosition(float Yaw)
{
	// @todo arkit implement FAppleARKitSystem::ResetOrientationAndPosition
}

bool FAppleARKitSystem::IsHeadTrackingAllowed() const
{
#if ARKIT_SUPPORT && __IPHONE_OS_VERSION_MAX_ALLOWED >= 110000
	if ([IOSAppDelegate GetDelegate].OSVersion >= 11.0f)
	{
        return GetDefault<UGeneralProjectSettings>()->bStartInAR;
	}
	else
	{
		return false;
	}
#else
	return false;
#endif //ARKIT_SUPPORT
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

void FAppleARKitSystem::OnBeginRendering_GameThread()
{
	UpdatePoses();
}

bool FAppleARKitSystem::OnStartGameFrame(FWorldContext& WorldContext)
{
	FXRTrackingSystemBase::OnStartGameFrame(WorldContext);
	
	CachedTrackingToWorld = ComputeTrackingToWorldTransform(WorldContext);

	
	return true;
}

//bool FAppleARKitSystem::ARLineTraceFromScreenPoint(const FVector2D ScreenPosition, TArray<FARTraceResult>& OutHitResults)
//{
//	const bool bSuccess = HitTestAtScreenPosition(ScreenPosition, EAppleARKitHitTestResultType::ExistingPlaneUsingExtent, OutHitResults);
//	return bSuccess;
//}

void FAppleARKitSystem::OnARSystemInitialized()
{
	// Register for device orientation changes
	FCoreDelegates::ApplicationReceivedScreenOrientationChangedNotificationDelegate.AddThreadSafeSP(this, &FAppleARKitSystem::OrientationChanged);
}

EARTrackingQuality FAppleARKitSystem::OnGetTrackingQuality() const
{
	return GameThreadFrame.IsValid()
		? GameThreadFrame->Camera.TrackingQuality
		: EARTrackingQuality::NotAvailable;
}

bool FAppleARKitSystem::OnStartAR()
{
	Run();
	return true;
}

void FAppleARKitSystem::OnStopAR()
{
	Pause();
}

static bool IsHitInRange( float ARKitHitDistnace, float WorldToMetersScale )
{
    // Skip results further than 5m or closer that 20cm from camera
	const float UnrealHitDistance = ARKitHitDistnace * WorldToMetersScale;
	return 20.0f < UnrealHitDistance && UnrealHitDistance < 500.0f;
}

#if ARKIT_SUPPORT && __IPHONE_OS_VERSION_MAX_ALLOWED >= 110000

static UARTrackedGeometry* FindGeometryFromAnchor( ARAnchor* InAnchor, TMap<FGuid, UARTrackedGeometry*>& Geometries )
{
	if (InAnchor != NULL)
	{
		const FGuid AnchorGUID = ToFGuid( InAnchor.identifier );
		UARTrackedGeometry** Result = Geometries.Find(AnchorGUID);
		if (Result != nullptr)
		{
			return *Result;
		}
	}
	
	return nullptr;
}

#endif//ARKIT_SUPPORT && __IPHONE_OS_VERSION_MAX_ALLOWED >= 110000

TArray<FARTraceResult> FAppleARKitSystem::OnLineTraceTrackedObjects( const FVector2D ScreenCoord )
{
	const float WorldToMetersScale = GetWorldToMetersScale();
	TArray<FARTraceResult> Results;
	
	// Sanity check
	if (IsRunning())
	{
#if ARKIT_SUPPORT && __IPHONE_OS_VERSION_MAX_ALLOWED >= 110000
		
		TSharedRef<FARSystemBase, ESPMode::ThreadSafe> This = SharedThis(this);
		
		@autoreleasepool
		{
			// Perform a hit test on the Session's last frame
			ARFrame* HitTestFrame = Session.currentFrame;
			if (HitTestFrame)
			{
				Results.Reserve(8);
				
				// Convert the screen position to normalised coordinates in the capture image space
				FVector2D NormalizedImagePosition = FAppleARKitCamera( HitTestFrame.camera ).GetImageCoordinateForScreenPosition( ScreenCoord, EAppleARKitBackgroundFitMode::Fill );
				switch (DeviceOrientation)
				{
					case EScreenOrientation::Portrait:
						NormalizedImagePosition = FVector2D( NormalizedImagePosition.Y, 1.0f - NormalizedImagePosition.X );
						break;
						
					case EScreenOrientation::PortraitUpsideDown:
						NormalizedImagePosition = FVector2D( 1.0f - NormalizedImagePosition.Y, NormalizedImagePosition.X );
						break;
						
					default:
					case EScreenOrientation::LandscapeLeft:
						break;
						
					case EScreenOrientation::LandscapeRight:
						NormalizedImagePosition = FVector2D(1.0f, 1.0f) - NormalizedImagePosition;
						break;
				};
				
				// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::Printf(TEXT("Hit Test At Screen Position: x: %f, y: %f"), NormalizedImagePosition.X, NormalizedImagePosition.Y));
				
				// First run hit test against existing planes with extents (converting & filtering results as we go)
				NSArray< ARHitTestResult* >* PlaneHitTestResults = [HitTestFrame hitTest:CGPointMake(NormalizedImagePosition.X, NormalizedImagePosition.Y) types:ARHitTestResultTypeExistingPlaneUsingExtent];
				for ( ARHitTestResult* HitTestResult in PlaneHitTestResults )
				{
					if ( IsHitInRange( HitTestResult.distance, WorldToMetersScale ) )
					{						
						// Hit result has passed and above filtering, add it to the list
						// Convert to Unreal's Hit Test result format
						Results.Add( FARTraceResult( This, FAppleARKitTransform::ToFTransform( HitTestResult.worldTransform, WorldToMetersScale ), FindGeometryFromAnchor(HitTestResult.anchor, TrackedGeometries) ) );
					}
				}
				
				// If there were no valid results, fall back to hit testing against one shot plane
				if (!Results.Num())
				{
					PlaneHitTestResults = [HitTestFrame hitTest:CGPointMake(NormalizedImagePosition.X, NormalizedImagePosition.Y) types:ARHitTestResultTypeEstimatedHorizontalPlane];
					for ( ARHitTestResult* HitTestResult in PlaneHitTestResults )
					{
						if ( IsHitInRange( HitTestResult.distance, WorldToMetersScale ) )
						{
							// Hit result has passed and above filtering, add it to the list
							// Convert to Unreal's Hit Test result format
							Results.Add( FARTraceResult( This, FAppleARKitTransform::ToFTransform( HitTestResult.worldTransform, WorldToMetersScale ), FindGeometryFromAnchor(HitTestResult.anchor, TrackedGeometries) ) );
						}
					}
				}
				
				// If there were no valid results, fall back further to hit testing against feature points
				if (!Results.Num())
				{
					// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("No results for plane hit test - reverting to feature points"), NormalizedImagePosition.X, NormalizedImagePosition.Y));
					
					NSArray< ARHitTestResult* >* FeatureHitTestResults = [HitTestFrame hitTest:CGPointMake(NormalizedImagePosition.X, NormalizedImagePosition.Y) types:ARHitTestResultTypeFeaturePoint];
					for ( ARHitTestResult* HitTestResult in FeatureHitTestResults )
					{
						if ( IsHitInRange( HitTestResult.distance, WorldToMetersScale ) )
						{
							// Hit result has passed and above filtering, add it to the list
							// Convert to Unreal's Hit Test result format
							Results.Add( FARTraceResult( This, FAppleARKitTransform::ToFTransform( HitTestResult.worldTransform, WorldToMetersScale ), FindGeometryFromAnchor(HitTestResult.anchor, TrackedGeometries) ) );
						}
					}
				}
			}
		}
#endif // ARKIT_SUPPORT
	}
	
	return Results;
}

TArray<UARTrackedGeometry*> FAppleARKitSystem::OnGetAllTrackedGeometries() const
{
	TArray<UARTrackedGeometry*> Geometries;
	TrackedGeometries.GenerateValueArray(Geometries);
	return Geometries;
}

bool FAppleARKitSystem::GetCurrentFrame(FAppleARKitFrame& OutCurrentFrame) const
{
	if( GameThreadFrame.IsValid() )
	{
		OutCurrentFrame = *GameThreadFrame;
		return true;
	}
	else
	{
		return false;
	}
}

void FAppleARKitSystem::AddReferencedObjects( FReferenceCollector& Collector )
{
	Collector.AddReferencedObjects( TrackedGeometries );
}

bool FAppleARKitSystem::HitTestAtScreenPosition(const FVector2D ScreenPosition, EAppleARKitHitTestResultType InTypes, TArray< FAppleARKitHitTestResult >& OutResults)
{
	ensureMsgf(false,TEXT("UNIMPLEMENTED; see OnLineTraceTrackedObjects()"));
	return false;
}

static TOptional<EScreenOrientation::Type> PickAllowedDeviceOrientation( EScreenOrientation::Type InOrientation )
{
#if ARKIT_SUPPORT && __IPHONE_OS_VERSION_MAX_ALLOWED >= 110000
	const UIOSRuntimeSettings* IOSSettings = GetDefault<UIOSRuntimeSettings>();
	
	const bool bOrientationSupported[] =
	{
		true, // Unknown
		IOSSettings->bSupportsPortraitOrientation != 0, // Portait
		IOSSettings->bSupportsUpsideDownOrientation != 0, // PortraitUpsideDown
		IOSSettings->bSupportsLandscapeRightOrientation != 0, // LandscapeLeft; These are flipped vs the enum name?
		IOSSettings->bSupportsLandscapeLeftOrientation != 0, // LandscapeRight; These are flipped vs the enum name?
		false, // FaceUp
		false // FaceDown
	};
	
	if (bOrientationSupported[static_cast<int32>(InOrientation)])
	{
		return InOrientation;
	}
	else
	{
		return TOptional<EScreenOrientation::Type>();
	}
#else
	return TOptional<EScreenOrientation::Type>();
#endif
}

void FAppleARKitSystem::SetDeviceOrientation( EScreenOrientation::Type InOrientation )
{
	TOptional<EScreenOrientation::Type> NewOrientation = PickAllowedDeviceOrientation(InOrientation);

	if (!NewOrientation.IsSet() && DeviceOrientation == EScreenOrientation::Unknown)
	{
		// We do not currently have a valid orientation, nor did the device provide one.
		// So pick ANY ALLOWED default.
		// This only realy happens if the device is face down on something or
		// in another "useless" state for AR.
		
		if (!NewOrientation.IsSet())
		{
			NewOrientation = PickAllowedDeviceOrientation(EScreenOrientation::Portrait);
		}
		
		if (!NewOrientation.IsSet())
		{
			NewOrientation = PickAllowedDeviceOrientation(EScreenOrientation::LandscapeLeft);
		}
		
		if (!NewOrientation.IsSet())
		{
			NewOrientation = PickAllowedDeviceOrientation(EScreenOrientation::PortraitUpsideDown);
		}
		
		if (!NewOrientation.IsSet())
		{
			NewOrientation = PickAllowedDeviceOrientation(EScreenOrientation::LandscapeRight);
		}
		
		check(NewOrientation.IsSet());
	}
	
	if (NewOrientation.IsSet() && DeviceOrientation != NewOrientation.GetValue())
	{
		DeviceOrientation = NewOrientation.GetValue();
		DerivedTrackingToUnrealRotation = DeriveTrackingToWorldRotation( DeviceOrientation );
	}
}


void FAppleARKitSystem::Run()
{
	// @todo arkit FAppleARKitSystem::GetWorldToMetersScale needs a real scale somehow
	FAppleARKitConfiguration Config;
	RunWithConfiguration( Config );
}

bool FAppleARKitSystem::RunWithConfiguration(const FAppleARKitConfiguration& InConfiguration)
{

	if (IsRunning())
	{
		UE_LOG(LogAppleARKit, Log, TEXT("Session already running"), this);

		return true;
	}

#if ARKIT_SUPPORT && __IPHONE_OS_VERSION_MAX_ALLOWED >= 110000
	if ([IOSAppDelegate GetDelegate].OSVersion >= 11.0f)
	{

		ARSessionRunOptions options = 0;

		// Create our ARSessionDelegate
		if (Delegate == nullptr)
		{
			Delegate = [[FAppleARKitSessionDelegate alloc] initWithAppleARKitSystem:this];
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
			[Delegate setMetalTextureCache : MetalTextureCache];
		}

		// Convert to native ARWorldTrackingSessionConfiguration
		ARConfiguration* Configuration = FAppleARKitConfiguration::ToARConfiguration(InConfiguration);

		UE_LOG(LogAppleARKit, Log, TEXT("Starting session: %p with options %d"), this, options);

		// Start the session with the configuration
		[Session runWithConfiguration : Configuration options : options];
	}
	
#endif // ARKIT_SUPPORT
	
	// @todo arkit Add support for relocating ARKit space to Unreal World Origin? BaseTransform = FTransform::Identity;
	
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
	
#if ARKIT_SUPPORT && __IPHONE_OS_VERSION_MAX_ALLOWED >= 110000
	if ([IOSAppDelegate GetDelegate].OSVersion >= 11.0f)
	{
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
	}
	
#endif // ARKIT_SUPPORT
	
	// Set running state
	bIsRunning = false;
	
	return true;
}

void FAppleARKitSystem::OrientationChanged(const int32 NewOrientationRaw)
{
	const EScreenOrientation::Type NewOrientation = static_cast<EScreenOrientation::Type>(NewOrientationRaw);
	SetDeviceOrientation(NewOrientation);
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

#if ARKIT_SUPPORT && __IPHONE_OS_VERSION_MAX_ALLOWED >= 110000

static TSharedPtr<FAppleARKitAnchorData> MakeAnchorData( ARAnchor* Anchor, const float WorldToMeterScale )
{
	// Construct appropriate UAppleARKitAnchor subclass
	TSharedPtr<FAppleARKitAnchorData> NewAnchor;
	if ([Anchor isKindOfClass:[ARPlaneAnchor class]])
	{
		ARPlaneAnchor* PlaneAnchor = (ARPlaneAnchor*)Anchor;
		NewAnchor = MakeShared<FAppleARKitAnchorData>(
			ToFGuid(PlaneAnchor.identifier),
			FAppleARKitTransform::ToFTransform(PlaneAnchor.transform, WorldToMeterScale),
			FAppleARKitTransform::ToFVector(PlaneAnchor.center, WorldToMeterScale),
			// @todo use World Settings WorldToMetersScale
			0.5f*FAppleARKitTransform::ToFVector(PlaneAnchor.extent, WorldToMeterScale).GetAbs()
		);
	}
	else
	{
		NewAnchor = MakeShared<FAppleARKitAnchorData>(
			ToFGuid(Anchor.identifier),
			FAppleARKitTransform::ToFTransform(Anchor.transform, WorldToMeterScale));
	}
	
	return NewAnchor;
}

void FAppleARKitSystem::SessionDidAddAnchors_DelegateThread( NSArray<ARAnchor*>* anchors )
{
	DECLARE_CYCLE_STAT(TEXT("FAppleARKitSystem::SessionDidAddAnchors_DelegateThread"),
					   STAT_FAppleARKitSystem_SessionDidAddAnchors,
					   STATGROUP_APPLEARKIT);

	for (ARAnchor* anchor in anchors)
	{
		TSharedPtr<FAppleARKitAnchorData> NewAnchorData = MakeAnchorData(anchor, GetWorldToMetersScale());
		if (ensure(NewAnchorData.IsValid()))
		{
			auto AddAnchorTask = FSimpleDelegateGraphTask::FDelegate::CreateSP(this, &FAppleARKitSystem::SessionDidAddAnchors_Internal, NewAnchorData.ToSharedRef());
			FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(AddAnchorTask, GET_STATID(STAT_FAppleARKitSystem_SessionDidAddAnchors), nullptr, ENamedThreads::GameThread);
		}
	}
}

void FAppleARKitSystem::SessionDidUpdateAnchors_DelegateThread( NSArray<ARAnchor*>* anchors )
{
	DECLARE_CYCLE_STAT(TEXT("FAppleARKitSystem::SessionDidUpdateAnchors_DelegateThread"),
					   STAT_FAppleARKitSystem_SessionDidUpdateAnchors,
					   STATGROUP_APPLEARKIT);
	
	for (ARAnchor* anchor in anchors)
	{
		TSharedPtr<FAppleARKitAnchorData> NewAnchorData = MakeAnchorData(anchor, GetWorldToMetersScale());
		if (ensure(NewAnchorData.IsValid()))
		{
			auto UpdateAnchorTask = FSimpleDelegateGraphTask::FDelegate::CreateSP(this, &FAppleARKitSystem::SessionDidUpdateAnchors_Internal, NewAnchorData.ToSharedRef());
			FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(UpdateAnchorTask, GET_STATID(STAT_FAppleARKitSystem_SessionDidUpdateAnchors), nullptr, ENamedThreads::GameThread);
		}
	}
}

void FAppleARKitSystem::SessionDidRemoveAnchors_DelegateThread( NSArray<ARAnchor*>* anchors )
{
	DECLARE_CYCLE_STAT(TEXT("FAppleARKitSystem::SessionDidRemoveAnchors_DelegateThread"),
					   STAT_FAppleARKitSystem_SessionDidRemoveAnchors,
					   STATGROUP_APPLEARKIT);
	
	for (ARAnchor* anchor in anchors)
	{
		// Convert to FGuid
		const FGuid AnchorGuid = ToFGuid( anchor.identifier );

		auto RemoveAnchorTask = FSimpleDelegateGraphTask::FDelegate::CreateSP(this, &FAppleARKitSystem::SessionDidRemoveAnchors_Internal, AnchorGuid);
		FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(RemoveAnchorTask, GET_STATID(STAT_FAppleARKitSystem_SessionDidRemoveAnchors), nullptr, ENamedThreads::GameThread);
	}
}

void FAppleARKitSystem::SessionDidAddAnchors_Internal( TSharedRef<FAppleARKitAnchorData> AnchorData )
{
	const float WorldToMetersScale = GetWorldToMetersScale();
	
	UARPlaneGeometry* NewGeometry = NewObject<UARPlaneGeometry>();
	NewGeometry->UpdateTrackedGeometry(SharedThis(this), AnchorData->Transform, AnchorData->Center, AnchorData->Extent);
	
	UARTrackedGeometry* NewTrackedGeometry = TrackedGeometries.Add( AnchorData->AnchorGUID, NewGeometry );
}

void FAppleARKitSystem::SessionDidUpdateAnchors_Internal( TSharedRef<FAppleARKitAnchorData> AnchorData )
{
	UARTrackedGeometry** FoundGeometry = TrackedGeometries.Find(AnchorData->AnchorGUID);
	UARPlaneGeometry* ExistingGeometry = (FoundGeometry != nullptr)
		? Cast<UARPlaneGeometry>(*FoundGeometry)
		: nullptr;
	
	if (ensure(ExistingGeometry != nullptr))
	{
		ExistingGeometry->UpdateTrackedGeometry(SharedThis(this), AnchorData->Transform, AnchorData->Center, AnchorData->Extent);
	};
}

void FAppleARKitSystem::SessionDidRemoveAnchors_Internal( FGuid AnchorGuid )
{
	TrackedGeometries.Remove(AnchorGuid);
}

#endif //ARKIT_SUPPORT






namespace AppleARKitSupport
{
	TSharedPtr<class FAppleARKitSystem, ESPMode::ThreadSafe> CreateAppleARKitSystem()
	{
#if ARKIT_SUPPORT && __IPHONE_OS_VERSION_MAX_ALLOWED >= 110000
		const bool bIsARApp = GetDefault<UGeneralProjectSettings>()->bStartInAR;
		if (bIsARApp)
		{
			auto NewARKitSystem = NewARSystem<FAppleARKitSystem>();
			return NewARKitSystem;;
		}
#endif
		return TSharedPtr<class FAppleARKitSystem, ESPMode::ThreadSafe>();
	}
}
