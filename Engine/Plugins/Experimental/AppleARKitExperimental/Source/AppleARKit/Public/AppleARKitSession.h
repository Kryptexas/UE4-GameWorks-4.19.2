#pragma once


// UE4
#include "Templates/SharedPointer.h"
#include "HAL/CriticalSection.h"
#include "Engine/EngineTypes.h"
#include "UObject/Object.h"

// C++
#include <atomic>

// ARKit
#if ARKIT_SUPPORT
#import <ARKit/ARKit.h>
#endif // ARKIT_SUPPORT

// AppleARKit
#include "AppleARKitFrame.h"
#include "AppleARKitSessionDelegate.h"
#include "AppleARKitConfiguration.h"
#include "AppleARKitHitTestResult.h"
#include "AppleARKitAnchor.h"
#include "AppleARKitSession.generated.h"

/**
 * The UAppleARKitSession class configures and tracks running different Augmented Reality techniques on 
 * a device.
 */
UCLASS( BlueprintType )
class UAppleARKitSession : public UObject
{
	GENERATED_BODY()

public: 

	// Destructor. Calls Stop()
	virtual ~UAppleARKitSession();

	/** 
	 * Start / resume the session with default world tracking configuration.
	 *
	 * Default configuration is the UAppleARKitWorldTrackingSessionConfiguration CDO.
	 *
	 * @see RunWithConfiguration, UAppleARKitWorldTrackingSessionConfiguration
	 */
	UFUNCTION( BlueprintCallable, Category="AppleARKit|Session" )
	bool Run();

	/**
	 * Runs the session with the provided configuration.
	 * 
	 * Calling run on a session that has already started will transition immediately to 
	 * using the new session configuration.
	 *
	 * @param Configuration The configuration to use.
	 * @see Run()
	 */
	UFUNCTION( BlueprintCallable, Category="AppleARKit|Session" )
	bool RunWithConfiguration( const UAppleARKitConfiguration* Configuration );

	/** Stop / halt the session */
	UFUNCTION( BlueprintCallable, Category="AppleARKit|Session" )
	bool Pause();

	/** enable / disable plane detection */
	UFUNCTION(BlueprintCallable, Category = "AppleARKit|Session")
	void EnablePlaneDetection(bool bOnOff);

	/** Returns true if the session was started successfully and is currently active */
	UFUNCTION( BlueprintPure, Category="AppleARKit|Session" )
	bool IsRunning() const { return bIsRunning; }

    UPROPERTY( BlueprintReadWrite, Category="AppleARKit|Session" )
    bool bEnableStats = true;
    
	/** 
	 * Returns the last processed FAppleARKitFrame from the session.
	 *
	 * When called from the game thread, we ensure a single frame is returned for the duration
	 * of the game thread update by watching for changes to GFrameNumber to trigger pulling a 
	 * new frame from the session.
	 */
	UFUNCTION( BlueprintCallable, Category="AppleARKit|Session", meta=(DisplayName="Get Current Frame") )
    bool K2_GetCurrentFrame( FAppleARKitFrame& OutCurrentFrame );


	void UpdateCurrentGameThreadFrame();

	/** 
	 * Returns the last processed FAppleARKitFrame from the session.
	 *
	 * When called from the game thread, we ensure a single frame is returned for the duration
	 * of the game thread update by watching for changes to GFrameNumber to trigger pulling a 
	 * new frame from the session.
	 */
	TSharedPtr< FAppleARKitFrame, ESPMode::ThreadSafe > GetCurrentFrame_GameThread();

	/**
	* Return the frame used to generate the camera matrices for the currently rendering frame.	
	*/
	TSharedPtr< FAppleARKitFrame, ESPMode::ThreadSafe > GetCurrentFrame_RenderThread();

	//unconditionally returns the latest frame available on any thread.
	TSharedPtr< FAppleARKitFrame, ESPMode::ThreadSafe > GetMostRecentFrame();

	// /** Returns the last processed FAppleARKitFrame from the session */
	// TSharedPtr< FAppleARKitFrame, ESPMode::ThreadSafe > GetLastReceivedFrame() const { return LastReceivedFrame; }

	/**
	 * Sets the base transform (defaults to identity) that is internally applied to all ARKit camera 
	 * & hit test transforms.
	 *
	 * This can be used for things like tap-to-place where you want to transform the ARKit world 
	 * space into your level space.
	 */
	UFUNCTION( BlueprintCallable, Category="AppleARKit|Session" )
	void SetBaseTransform( const FTransform& BaseTransform );

	/**
	 * Returns the base transform (defaults to identity) that is internally applied to all ARKit 
	 * camera & hit test transforms.
	 * @see SetBaseTransform
	 */
	UFUNCTION( BlueprintPure, Category="AppleARKit|Session" )
	FTransform GetBaseTransform() const { return BaseTransform; }

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

	/** Thread safe anchor getter */
	UAppleARKitAnchor* GetAnchor( const FGuid& Identifier ) const;

	/** Thread safe anchor map getter */
	TMap< FGuid, UAppleARKitAnchor* > GetAnchors() const;

	/** Thread safe anchor map getter */
	UFUNCTION( BlueprintCallable, Category="AppleARKit|Session", meta=(DisplayName="Get Anchors") )
	void K2_GetAnchors( TArray< UAppleARKitAnchor* >& OutAnchors ) const
	{
		GetAnchors().GenerateValueArray( OutAnchors );
	}
    
	UPROPERTY( BlueprintReadOnly, Category = "AppleARKitSession")
	float AverageUpdateInterval;
	
	UPROPERTY( BlueprintReadOnly, Category = "AppleARKitSession")
	float AverageCameraTransformUpdateInterval;

	// Session delegate callbacks
	void SessionDidUpdateFrame_DelegateThread( TSharedPtr< FAppleARKitFrame, ESPMode::ThreadSafe > Frame );
	void SessionDidFailWithError_DelegateThread( const FString& Error );
#if ARKIT_SUPPORT
	void SessionDidAddAnchors_DelegateThread( NSArray<ARAnchor*>* anchors );
	void SessionDidUpdateAnchors_DelegateThread( NSArray<ARAnchor*>* anchors );
	void SessionDidRemoveAnchors_DelegateThread( NSArray<ARAnchor*>* anchors );
#endif // ARKIT_SUPPORT
    
    //enqueues a renderthread command to set the RT frame at the right time.
    void MarshallRenderThreadFrame(TSharedPtr< FAppleARKitFrame, ESPMode::ThreadSafe > Frame);

	//actually performs the set on the renderthread
	void SetRenderThreadFrame(TSharedPtr< FAppleARKitFrame, ESPMode::ThreadSafe > Frame);

#if ARKIT_SUPPORT
	/** Interal ARKit ARSession this is wrapping */
	ARSession* GetARSession() const 
	{
		return Session;
	}
#endif // ARKIT_SUPPORT

private:

	// The frame number when LastReceivedFrame was last updated
	uint32 GameThreadFrameNumber;

	//'threadsafe' sharedptrs merely guaranteee atomicity when adding/removing refs.  You can still have a race
	//with destruction and copying sharedptrs.
	FCriticalSection FrameLock;

	// Last frame grabbed & processed by via the ARKit session delegate
	TSharedPtr< FAppleARKitFrame, ESPMode::ThreadSafe > GameThreadFrame;
	TSharedPtr< FAppleARKitFrame, ESPMode::ThreadSafe > RenderThreadFrame;
	TSharedPtr< FAppleARKitFrame, ESPMode::ThreadSafe > LastReceivedFrame;

	// Internal list of current known anchors
	mutable FCriticalSection AnchorsLock;
	UPROPERTY( Transient )
	TMap< FGuid, UAppleARKitAnchor* > Anchors;

	// The base transform to apply to all ARKit transforms
	// @see SetBaseTransform
	FTransform BaseTransform;

	// Average update interval timing
	uint32 LastUpdateTimestamp = 0;
	uint32 LastCameraTransformTimestamp = 0;

	// Whether or not the session has started (successfully)
	bool bIsRunning = false;

#if ARKIT_SUPPORT

	// ARKit Session
	ARSession* Session = nullptr;

	// ARKit Session Delegate
	FAppleARKitSessionDelegate* Delegate = nullptr;

	/** The Metal texture cache for unbuffered texture uploads. */
	CVMetalTextureCacheRef MetalTextureCache = nullptr;

#endif // ARKIT_SUPPORT

};
