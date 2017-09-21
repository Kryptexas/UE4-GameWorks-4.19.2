// AppleARKit
#include "AppleARKitSession.h"
#include "AppleARKit.h"
#include "AppleARKitStats.h"
#include "AppleARKitPlaneAnchor.h"

// UE4
#include "CoreGlobals.h"
#include "Misc/ScopeLock.h"
#include "Kismet/GameplayStatics.h"
#include "RenderingCommon.h"
#include "RenderingThread.h"

UAppleARKitSession::~UAppleARKitSession()
{
	// Ensure the session is cleanly stopped
	Pause();
}

bool UAppleARKitSession::Run()
{
	// Use UAppleARKitWorldTrackingSessionConfiguration's CDO for configuration
	UAppleARKitWorldTrackingSessionConfiguration* Configuration = UAppleARKitWorldTrackingSessionConfiguration::StaticClass()->GetDefaultObject< UAppleARKitWorldTrackingSessionConfiguration >();

	// Run with that
	return RunWithConfiguration(Configuration);
}

bool UAppleARKitSession::RunWithConfiguration(const UAppleARKitSessionConfiguration* InConfiguration)
{
	// Already running?
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
		Delegate =[[FAppleARKitSessionDelegate alloc] initWithAppleARKitSession:this];
	}

	if (Session == nullptr)
	{
        // Start a new ARSession
        Session =[ARSession new];
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
	ARConfiguration* Configuration = ToARSessionConfiguration(InConfiguration);

    UE_LOG(LogAppleARKit, Log, TEXT("Starting session: %p with options %d"), this, options);
    
	// Start the session with the configuration
    [Session runWithConfiguration:Configuration options:options];

#endif // ARKIT_SUPPORT

    BaseTransform = FTransform::Identity;
    
	// Set running state
	bIsRunning = true;

	return true;
}

void UAppleARKitSession::EnablePlaneDetection(bool bOnOff)
{
	// yes, this is gross.

}


bool UAppleARKitSession::Pause()
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

bool UAppleARKitSession::K2_GetCurrentFrame(FAppleARKitFrame& OutCurrentFrame)
{
	FScopeLock ScopeLock(&FrameLock);
	TSharedPtr< FAppleARKitFrame, ESPMode::ThreadSafe > CurrentFrame = GetCurrentFrame_GameThread();

	// Return copy (for thread safety). Here we make sure to hold a thread-safe reference to
	// the current frame while we do the copy.
	if (CurrentFrame.IsValid())
	{
		OutCurrentFrame = *CurrentFrame.Get();
		return true;
	}
    
	return false;
}


void UAppleARKitSession::UpdateCurrentGameThreadFrame()
{
	// When called from the game thread, we ensure a single frame is returned for the duration
	// of the game thread update by watching for changes to GFrameNumber to trigger pulling a 
	// new frame from the session.
	check(IsInGameThread());
	if (IsRunning())
	{
		FScopeLock ScopeLock(&FrameLock);

		// Are we in a new game thread frame? Grab the next session frame
		if (GameThreadFrameNumber != GFrameNumber || !LastReceivedFrame.IsValid())
		{
#if ARKIT_SUPPORT
			// Is there a new frame to pull?
			if (GameThreadFrame.Get() == LastReceivedFrame.Get())
			{
				UE_LOG(LogAppleARKit, Verbose, TEXT("Game thread requested new frame with no new frames received from the ARSessionDelegate"));
			}
			else
			{
				// After checking the above, we know we now have a new frame that is newer than our 
				// last (or our first frame ever).
				UE_LOG(LogAppleARKit, Verbose, TEXT("Updating game thread frame"));

				// Store new game thread frame
				GameThreadFrame = LastReceivedFrame;

				// Apply BaseTransform to the frame's camera transform
				GameThreadFrame->Camera.Transform = (GameThreadFrame->Camera.Transform * BaseTransform);
			}
#endif // ARKIT_SUPPORT

			// We still update the frame number here, even if we didn't actually pull a new frame 
			// above. This is to ensure we don't try and pull a new frame again on this game thread
			// frame, which would allow different game thread components to see different states 
			// throughout the frame.
			GameThreadFrameNumber = GFrameNumber;
		}
	}
}

TSharedPtr< FAppleARKitFrame, ESPMode::ThreadSafe > UAppleARKitSession::GetCurrentFrame_GameThread()
{
	// When called from the game thread, we ensure a single frame is returned for the duration
	// of the game thread update by watching for changes to GFrameNumber to trigger pulling a 
	// new frame from the session.
	check(IsInGameThread());
	return GameThreadFrame;
}

TSharedPtr< FAppleARKitFrame, ESPMode::ThreadSafe > UAppleARKitSession::GetCurrentFrame_RenderThread()
{
	check(IsInRenderingThread());
	return RenderThreadFrame;
}

TSharedPtr< FAppleARKitFrame, ESPMode::ThreadSafe > UAppleARKitSession::GetMostRecentFrame()
{
	FScopeLock ScopeLock(&FrameLock);
	TSharedPtr< FAppleARKitFrame, ESPMode::ThreadSafe > CurrentLastFrame = LastReceivedFrame;
	return CurrentLastFrame;
}

//enqueues a renderthread command to set the RT frame at the right time.
void UAppleARKitSession::MarshallRenderThreadFrame(TSharedPtr< FAppleARKitFrame, ESPMode::ThreadSafe > Frame)
{
	check(IsInGameThread());
	typedef TSharedPtr<FAppleARKitFrame, ESPMode::ThreadSafe> AppleARKitSafeFrame;

	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		MarshallAppleARKitRenderThreadFrame,
		UAppleARKitSession*, Session, this,
		AppleARKitSafeFrame, NewFrame, Frame,
		{
			Session->SetRenderThreadFrame(NewFrame);
		});
}

//actually performs the set on the renderthread
void UAppleARKitSession::SetRenderThreadFrame(TSharedPtr< FAppleARKitFrame, ESPMode::ThreadSafe > Frame)
{
	check(IsInRenderingThread());
	RenderThreadFrame = Frame;
}

void UAppleARKitSession::SetBaseTransform(const FTransform& InBaseTransform)
{
	BaseTransform = InBaseTransform;
}

bool UAppleARKitSession::HitTestAtScreenPosition(const FVector2D ScreenPosition, EAppleARKitHitTestResultType InTypes, TArray< FAppleARKitHitTestResult >& OutResults)
{
	// Sanity check
	if (!IsRunning())
	{
		return false;
	}

	// Clear the HitResults
	OutResults.Empty();

#if ARKIT_SUPPORT

	@autoreleasepool {

		// Perform a hit test on the Session's last frame
		ARFrame* HitTestFrame = Session.currentFrame;
		if (!HitTestFrame)
		{
			return false;
		}

		// Convert the screen position to normalised coordinates in the capture image space
		FVector2D NormalizedImagePosition = FAppleARKitCamera( HitTestFrame.camera ).GetImageCoordinateForScreenPosition( ScreenPosition, EAppleARKitBackgroundFitMode::Fill );

		// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::Printf(TEXT("Hit Test At Screen Position: x: %f, y: %f"), NormalizedImagePosition.X, NormalizedImagePosition.Y));

		// Convert the types flags
		ARHitTestResultType Types = ToARHitTestResultType( InTypes );

		// First run hit test against existing planes with extents (converting & filtering results as we go)
		NSArray< ARHitTestResult* >* PlaneHitTestResults = [HitTestFrame hitTest:CGPointMake(NormalizedImagePosition.X, NormalizedImagePosition.Y) types:ARHitTestResultTypeExistingPlaneUsingExtent];
		for ( ARHitTestResult* HitTestResult in PlaneHitTestResults )
		{
			// Convert to FAppleARKitHitTestResult
			FAppleARKitHitTestResult OutResult( HitTestResult );

			// Skip results further than 5m or closer that 20cm from camera
			if (OutResult.Distance > 500.0f || OutResult.Distance < 20.0f)
			{
				continue;
			}

			// Apply BaseTransform
			OutResult.Transform *= BaseTransform;

			// Hit result has passed and above filtering, add it to the list
			OutResults.Add( OutResult );
		}

		// If there were no valid results, fall back to hit testing against one shot plane
		if (!OutResults.Num())
		{
			PlaneHitTestResults = [HitTestFrame hitTest:CGPointMake(NormalizedImagePosition.X, NormalizedImagePosition.Y) types:ARHitTestResultTypeEstimatedHorizontalPlane];
			for ( ARHitTestResult* HitTestResult in PlaneHitTestResults )
			{
				// Convert to FAppleARKitHitTestResult
				FAppleARKitHitTestResult OutResult( HitTestResult );

				// Skip results further than 5m or closer that 20cm from camera
				if (OutResult.Distance > 500.0f || OutResult.Distance < 20.0f)
				{
					continue;
				}

				// Apply BaseTransform
				OutResult.Transform *= BaseTransform;

				// Hit result has passed and above filtering, add it to the list
				OutResults.Add( OutResult );
			}
		}

		// If there were no valid results, fall back further to hit testing against feature points
		if (!OutResults.Num())
		{
			// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("No results for plane hit test - reverting to feature points"), NormalizedImagePosition.X, NormalizedImagePosition.Y));

			NSArray< ARHitTestResult* >* FeatureHitTestResults = [HitTestFrame hitTest:CGPointMake(NormalizedImagePosition.X, NormalizedImagePosition.Y) types:ARHitTestResultTypeFeaturePoint];
			for ( ARHitTestResult* HitTestResult in FeatureHitTestResults )
			{
				// Convert to FAppleARKitHitTestResult
				FAppleARKitHitTestResult OutResult( HitTestResult );

				// Skip results further than 5m or closer that 20cm from camera
				if (OutResult.Distance > 500.0f || OutResult.Distance < 20.0f)
				{
					continue;
				}

				// Apply BaseTransform
				OutResult.Transform *= BaseTransform;

				// Hit result has passed and above filtering, add it to the list
				OutResults.Add( OutResult );
			}
		}

		// if (!OutResults.Num())
		// {
		// 	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("No results for feature points either!"), NormalizedImagePosition.X, NormalizedImagePosition.Y));
		// }
		// else
		// {
		// 	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Found %d hit results, first at a distance of %fcm"), OutResults[0].Distance));
		// }
	}

#endif // ARKIT_SUPPORT

	return (OutResults.Num() > 0);
}

void UAppleARKitSession::SessionDidUpdateFrame_DelegateThread(TSharedPtr< FAppleARKitFrame, ESPMode::ThreadSafe > Frame)
{
	// Thread safe swap buffered frame
	FScopeLock ScopeLock(&FrameLock);
	LastReceivedFrame = Frame;
}

void UAppleARKitSession::SessionDidFailWithError_DelegateThread(const FString& Error)
{
	UE_LOG(LogAppleARKit, Warning, TEXT("Session failed with error: %s"), *Error);
}

UAppleARKitAnchor* UAppleARKitSession::GetAnchor( const FGuid& Identifier ) const
{
	FScopeLock ScopeLock( &AnchorsLock );

	if ( UAppleARKitAnchor*const* Anchor = Anchors.Find( Identifier ) )
	{
		return *Anchor;	
	}

	return nullptr;
}

TMap< FGuid, UAppleARKitAnchor* > UAppleARKitSession::GetAnchors() const
{
	FScopeLock ScopeLock( &AnchorsLock );

	return Anchors;
}

#if ARKIT_SUPPORT

FORCEINLINE void ToFGuid( uuid_t UUID, FGuid& OutGuid )
{
	// Set FGuid parts
	OutGuid.A = *(uint32*)UUID;
	OutGuid.B = *((uint32*)UUID)+1;
	OutGuid.C = *((uint32*)UUID)+2;
	OutGuid.D = *((uint32*)UUID)+3;
}

FORCEINLINE void ToFGuid( NSUUID* Identifier, FGuid& OutGuid )
{
	// Get bytes
	uuid_t UUID;
	[Identifier getUUIDBytes:UUID];

	// Set FGuid parts
	ToFGuid( UUID, OutGuid );
}

void UAppleARKitSession::SessionDidAddAnchors_DelegateThread( NSArray<ARAnchor*>* anchors )
{
	FScopeLock ScopeLock( &AnchorsLock );

	for (ARAnchor* anchor in anchors)
	{
		// Construct appropriate UAppleARKitAnchor subclass
		UAppleARKitAnchor* Anchor;
		if ([anchor isKindOfClass:[ARPlaneAnchor class]])
		{
			Anchor = NewObject< UAppleARKitPlaneAnchor >();
		}
		else
		{
			Anchor = NewObject< UAppleARKitAnchor >();
		}

		// Set UUID
		ToFGuid( anchor.identifier, Anchor->Identifier );

		// Update fields
		Anchor->Update_DelegateThread( anchor );

		// Map to UUID
		Anchors.Add( Anchor->Identifier, Anchor );
	}
}

void UAppleARKitSession::SessionDidUpdateAnchors_DelegateThread( NSArray<ARAnchor*>* anchors )
{
	FScopeLock ScopeLock( &AnchorsLock );

	for (ARAnchor* anchor in anchors)
	{
		// Convert to FGuid
		FGuid Identifier;
		ToFGuid( anchor.identifier, Identifier );

		// Lookup in map
		if ( UAppleARKitAnchor** Anchor = Anchors.Find( Identifier ) )
		{
			// Update fields
			(*Anchor)->Update_DelegateThread( anchor );
		}
	}
}

void UAppleARKitSession::SessionDidRemoveAnchors_DelegateThread( NSArray<ARAnchor*>* anchors )
{
	FScopeLock ScopeLock( &AnchorsLock );

	for (ARAnchor* anchor in anchors)
	{
		// Convert to FGuid
		FGuid Identifier;
		ToFGuid( anchor.identifier, Identifier );

		// Remove from map (allowing anchor to be garbage collected)
		Anchors.Remove( Identifier );
	}
}

#endif // ARKIT_SUPPORT
