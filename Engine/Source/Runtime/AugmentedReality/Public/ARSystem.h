// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Features/IModularFeature.h"
#include "XRTrackingSystemBase.h"
#include "ARTypes.h"
#include "ARSessionConfig.h"

class UARPin;
class UARTrackedGeometry;
class UARSessionConfig;
struct FARTraceResult;


/** Implement IARSystemSupport for any platform that wants to be an Unreal AR System. e.g. AppleARKit, GoogleARCore. */
class AUGMENTEDREALITY_API IARSystemSupport
{
public:
	/** Invoked after the base AR system has been initialized. */
	virtual void OnARSystemInitialized(){};

	/** @return the tracking quality; if unable to determine tracking quality, return EARTrackingQuality::NotAvailable */
	virtual EARTrackingQuality OnGetTrackingQuality() const = 0;
	
	/**
	 * Start the AR system.
	 *
	 * @param SessionType The type of AR session to create
	 *
	 * @return true if the system was successfully started
	 */
	virtual void OnStartARSession(UARSessionConfig* SessionConfig) = 0;

	/** Stop the AR system but leave its internal state intact. */
	virtual void OnPauseARSession() = 0;

	/** Stop the AR system and reset its internal state; this task must succeed. */
	virtual void OnStopARSession() = 0;

	/** @return the info about whether the session is running normally or encountered some kind of error. */
	virtual FARSessionStatus OnGetARSessionStatus() const = 0;
	
	/**
	 * Set a transform that will align the Tracking Space origin to the World Space origin.
	 * This is useful for supporting static geometry and static lighting in AR.
	 * Note: Usually, an app will ask the user to select an appropriate location for some
	 * experience. This allows us to choose an appropriate alignment transform.
	 */
	virtual void OnSetAlignmentTransform(const FTransform& InAlignmentTransform) = 0;
	
	/**
	 * Trace all the tracked geometries and determine which have been hit by a ray cast from `ScreenCoord`.
	 * Only geometries specified in `TraceChannels` are considered.
	 *
	 * @return a list of all the geometries that were hit, sorted by distance
	 */
	virtual TArray<FARTraceResult> OnLineTraceTrackedObjects( const FVector2D ScreenCoord, EARLineTraceChannels TraceChannels ) = 0;
	
	/** @return a TArray of all the tracked geometries known to your ar system */
	virtual TArray<UARTrackedGeometry*> OnGetAllTrackedGeometries() const = 0;
	
	/** @return a TArray of all the pins that attach components to TrackedGeometries */
	virtual TArray<UARPin*> OnGetAllPins() const = 0;

	/** @return whether the specified tracking type is supported by this device */
	virtual bool OnIsTrackingTypeSupported(EARSessionType SessionType) const = 0;

	/** @return the best available light estimate; nullptr if light estimation is inactive or not available */
	virtual UARLightEstimate* OnGetCurrentLightEstimate() const = 0;
	
	/**
	 * Pin an Unreal Component to a location in the world.
	 * Optionally, associate with a TrackedGeometry to receive transform updates that effectively attach the component to the geometry.
	 *
	 * @return the UARPin object that is pinning the component to the world and (optionally) a TrackedGeometry
	 */
	virtual UARPin* OnPinComponent(USceneComponent* ComponentToPin, const FTransform& PinToWorldTransform, UARTrackedGeometry* TrackedGeometry = nullptr, const FName DebugName = NAME_None) = 0;
	
	/**
	 * Given a pin, remove it and stop updating the associated component based on the tracked geometry.
	 * The component in question will continue to track with the world, but will not get updates specific to a TrackedGeometry.
	 */
	virtual void OnRemovePin(UARPin* PinToRemove) = 0;
	
	/** Unpin a component from any associated tracked geometry. This will remove the UARPin */
	virtual void OnRemovePin(USceneComponent* ComponentToUnpin) = 0;
	
public:
	virtual ~IARSystemSupport(){}
};

class AUGMENTEDREALITY_API FARSystemBase : public IARSystemSupport, public FXRTrackingSystemBase, public FGCObject, public TSharedFromThis<FARSystemBase, ESPMode::ThreadSafe>
{
public:
	//
	// MODULAR FEATURE SUPPORT
	//
	
	/**
	 * Part of the pattern for supporting modular features.
	 *
	 * @return the name of the feature.
	 */
	static FName GetModularFeatureName()
	{
		static const FName ModularFeatureName = FName(TEXT("ARSystem"));
		return ModularFeatureName;
	}

public:
	/** Control the construction of AR Systems. @see NewARSystem() */
	FARSystemBase();
	void InitializeARSystem();
	virtual ~FARSystemBase();

public:	
	EARTrackingQuality GetTrackingQuality() const;
	void StartARSession(UARSessionConfig* InSessionConfig);
	void PauseARSession();
	void StopARSession();
	FARSessionStatus GetARSessionStatus() const;
	bool IsSessionTypeSupported(EARSessionType SessionType) const;
	
	void SetAlignmentTransform( const FTransform& InAlignmentTransform );
	
	TArray<FARTraceResult> LineTraceTrackedObjects( const FVector2D ScreenCoords, EARLineTraceChannels TraceChannels );
	TArray<UARTrackedGeometry*> GetAllTrackedGeometries() const;
	TArray<UARPin*> GetAllPins() const;
	
	UARLightEstimate* GetCurrentLightEstimate() const;
	
	UARPin* PinComponent( USceneComponent* ComponentToPin, const FTransform& PinToWorldTransform, UARTrackedGeometry* TrackedGeometry = nullptr, const FName DebugName = NAME_None );
	UARPin* PinComponent( USceneComponent* ComponentToPin, const FARTraceResult& HitResult, const FName DebugName = NAME_None );
	void RemovePin( USceneComponent* ComponentToUnpin );
	void RemovePin( UARPin* PinToRemove );
	
public:
	const FTransform& GetAlignmentTransform() const;
	const UARSessionConfig& GetSessionConfig() const;
	UARSessionConfig& AccessSessionConfig();

protected:
	void SetAlignmentTransform_Internal(const FTransform& NewAlignmentTransform);

	//~ FGCObject
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	//~ FGCObject

private:
	/** Alignment transform between AR System's tracking space and Unreal's World Space. Useful in static lighting/geometry scenarios. */
	FTransform AlignmentTransform;

	UARSessionConfig* ARSettings;
};

template<class T, typename... ArgTypes>
TSharedRef<T, ESPMode::ThreadSafe> NewARSystem( ArgTypes&&... Args )
{
	auto NewARSystem = MakeShared<T, ESPMode::ThreadSafe>( Forward<ArgTypes>(Args)... );
	NewARSystem->InitializeARSystem();
	return NewARSystem;
}


